/// \file       index.c
/// \brief      Handling of .xz Indexes and some other Stream information
//  Author:     Lasse Collin
//
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#include "common.h"
#pragma hdrstop

struct lzma_index_encoder_coder {
	enum _Seq {
		SEQ_INDICATOR,
		SEQ_COUNT,
		SEQ_UNPADDED,
		SEQ_UNCOMPRESSED,
		SEQ_NEXT,
		SEQ_PADDING,
		SEQ_CRC32,
	} sequence;
	const lzma_index * index; /// Index being encoded
	lzma_index_iter iter; /// Iterator for the Index being encoded
	size_t pos; /// Position in integers
	uint32_t crc32; /// CRC32 of the List of Records field
};

struct lzma_index_decoder_coder {
	enum _Seq {
		SEQ_INDICATOR,
		SEQ_COUNT,
		SEQ_MEMUSAGE,
		SEQ_UNPADDED,
		SEQ_UNCOMPRESSED,
		SEQ_PADDING_INIT,
		SEQ_PADDING,
		SEQ_CRC32,
	} sequence;
	uint64_t memlimit; /// Memory usage limit
	lzma_index * index; /// Target Index
	lzma_index ** index_ptr; /// Pointer give by the application, which is set after successful decoding.
	lzma_vli count; /// Number of Records left to decode.
	lzma_vli unpadded_size; /// The most recent Unpadded Size field
	lzma_vli uncompressed_size; /// The most recent Uncompressed Size field
	size_t pos; /// Position in integers
	uint32_t crc32; /// CRC32 of the List of Records field
};


/// \brief      How many Records to allocate at once
///
/// This should be big enough to avoid making lots of tiny allocations
/// but small enough to avoid too much unused memory at once.
#define INDEX_GROUP_SIZE 512

/// \brief      How many Records can be allocated at once at maximum
#define PREALLOC_MAX ((SIZE_MAX - sizeof(index_group)) / sizeof(index_record))

/// \brief      Base structure for index_stream and index_group structures
typedef struct index_tree_node_s index_tree_node;
struct index_tree_node_s {
	/// Uncompressed start offset of this Stream (relative to the
	/// beginning of the file) or Block (relative to the beginning of the Stream)
	lzma_vli uncompressed_base;
	lzma_vli compressed_base; /// Compressed start offset of this Stream or Block
	index_tree_node * parent;
	index_tree_node * left;
	index_tree_node * right;
};

/// \brief      AVL tree to hold index_stream or index_group structures
struct index_tree {
	index_tree_node * root; /// Root node
	/// Leftmost node. Since the tree will be filled sequentially,
	/// this won't change after the first node has been added to the tree.
	index_tree_node * leftmost;
	/// The rightmost node in the tree. Since the tree is filled
	/// sequentially, this is always the node where to add the new data.
	index_tree_node * rightmost;
	uint32_t count; /// Number of nodes in the tree
};

struct index_record {
	lzma_vli uncompressed_sum;
	lzma_vli unpadded_sum;
};

struct index_group {
	index_tree_node node; /// Every Record group is part of index_stream.groups tree.
	lzma_vli number_base; /// Number of Blocks in this Stream before this group.
	size_t allocated; /// Number of Records that can be put in records[].
	size_t last; /// Index of the last Record in use.
	/// The sizes in this array are stored as cumulative sums relative
	/// to the beginning of the Stream. This makes it possible to
	/// use binary search in lzma_index_locate().
	///
	/// Note that the cumulative summing is done specially for
	/// unpadded_sum: The previous value is rounded up to the next
	/// multiple of four before adding the Unpadded Size of the new
	/// Block. The total encoded size of the Blocks in the Stream
	/// is records[last].unpadded_sum in the last Record group of
	/// the Stream.
	///
	/// For example, if the Unpadded Sizes are 39, 57, and 81, the
	/// stored values are 39, 97 (40 + 57), and 181 (100 + 181).
	/// The total encoded size of these Blocks is 184.
	///
	/// This is a flexible array, because it makes easy to optimize
	/// memory usage in case someone concatenates many Streams that
	/// have only one or few Blocks.
	index_record records[];
};

struct index_stream {
	index_tree_node node; /// Every index_stream is a node in the tree of Streams.
	uint32_t number; /// Number of this Stream (first one is 1)
	lzma_vli block_number_base; /// Total number of Blocks before this Stream
	/// Record groups of this Stream are stored in a tree.
	/// It's a T-tree with AVL-tree balancing. There are
	/// INDEX_GROUP_SIZE Records per node by default.
	/// This keeps the number of memory allocations reasonable
	/// and finding a Record is fast.
	index_tree groups;
	lzma_vli record_count; /// Number of Records in this Stream
	/// Size of the List of Records field in this Stream. This is used
	/// together with record_count to calculate the size of the Index
	/// field and thus the total size of the Stream.
	lzma_vli index_list_size;
	/// Stream Flags of this Stream. This is meaningful only if
	/// the Stream Flags have been told us with lzma_index_stream_flags().
	/// Initially stream_flags.version is set to UINT32_MAX to indicate
	/// that the Stream Flags are unknown.
	lzma_stream_flags stream_flags;
	/// Amount of Stream Padding after this Stream. This defaults to
	/// zero and can be set with lzma_index_stream_padding().
	lzma_vli stream_padding;
};

struct lzma_index_s {
	/// AVL-tree containing the Stream(s). Often there is just one
	/// Stream, but using a tree keeps lookups fast even when there
	/// are many concatenated Streams.
	index_tree streams;
	lzma_vli uncompressed_size; /// Uncompressed size of all the Blocks in the Stream(s)
	lzma_vli total_size; /// Total size of all the Blocks in the Stream(s)
	lzma_vli record_count; /// Total number of Records in all Streams in this lzma_index
	/// Size of the List of Records field if all the Streams in this
	/// lzma_index were packed into a single Stream (makes it simpler to
	/// take many .xz files and combine them into a single Stream).
	///
	/// This value together with record_count is needed to calculate
	/// Backward Size that is stored into Stream Footer.
	lzma_vli index_list_size;
	/// How many Records to allocate at once in lzma_index_append().
	/// This defaults to INDEX_GROUP_SIZE but can be overridden with
	/// lzma_index_prealloc().
	size_t prealloc;
	/// Bitmask indicating what integrity check types have been used
	/// as set by lzma_index_stream_flags(). The bit of the last Stream
	/// is not included here, since it is possible to change it by
	/// calling lzma_index_stream_flags() again.
	uint32_t checks;
};

static void index_tree_init(index_tree * tree)
{
	tree->root = NULL;
	tree->leftmost = NULL;
	tree->rightmost = NULL;
	tree->count = 0;
}

/// Helper for index_tree_end()
static void index_tree_node_end(index_tree_node * node, const lzma_allocator * allocator, void (*free_func)(void * node, const lzma_allocator * allocator))
{
	// The tree won't ever be very huge, so recursion should be fine.
	// 20 levels in the tree is likely quite a lot already in practice.
	if(node->left != NULL)
		index_tree_node_end(node->left, allocator, free_func);
	if(node->right != NULL)
		index_tree_node_end(node->right, allocator, free_func);
	free_func(node, allocator);
}

/// Free the memory allocated for a tree. Each node is freed using the
/// given free_func which is either &lzma_free or &index_stream_end.
/// The latter is used to free the Record groups from each index_stream
/// before freeing the index_stream itself.
static void index_tree_end(index_tree * tree, const lzma_allocator * allocator, void (*free_func)(void * node, const lzma_allocator * allocator))
{
	assert(free_func != NULL);
	if(tree->root != NULL)
		index_tree_node_end(tree->root, allocator, free_func);
}

/// Add a new node to the tree. node->uncompressed_base and
/// node->compressed_base must have been set by the caller already.
static void index_tree_append(index_tree * tree, index_tree_node * node)
{
	node->parent = tree->rightmost;
	node->left = NULL;
	node->right = NULL;
	++tree->count;
	// Handle the special case of adding the first node.
	if(tree->root == NULL) {
		tree->root = node;
		tree->leftmost = node;
		tree->rightmost = node;
		return;
	}
	// The tree is always filled sequentially.
	assert(tree->rightmost->uncompressed_base <= node->uncompressed_base);
	assert(tree->rightmost->compressed_base < node->compressed_base);

	// Add the new node after the rightmost node. It's the correct
	// place due to the reason above.
	tree->rightmost->right = node;
	tree->rightmost = node;

	// Balance the AVL-tree if needed. We don't need to keep the balance
	// factors in nodes, because we always fill the tree sequentially,
	// and thus know the state of the tree just by looking at the node
	// count. From the node count we can calculate how many steps to go
	// up in the tree to find the rotation root.
	uint32_t up = tree->count ^ (UINT32_C(1) << bsr32(tree->count));
	if(up != 0) {
		// Locate the root node for the rotation.
		up = ctz32(tree->count) + 2;
		do {
			node = node->parent;
		} while(--up > 0);
		// Rotate left using node as the rotation root.
		index_tree_node * pivot = node->right;
		if(node->parent == NULL) {
			tree->root = pivot;
		}
		else {
			assert(node->parent->right == node);
			node->parent->right = pivot;
		}
		pivot->parent = node->parent;
		node->right = pivot->left;
		if(node->right != NULL)
			node->right->parent = node;
		pivot->left = node;
		node->parent = pivot;
	}
}

/// Get the next node in the tree. Return NULL if there are no more nodes.
static void * index_tree_next(const index_tree_node * node)
{
	if(node->right != NULL) {
		node = node->right;
		while(node->left != NULL)
			node = node->left;
		return (void *)(node);
	}
	while(node->parent != NULL && node->parent->right == node)
		node = node->parent;
	return (void *)(node->parent);
}

/// Locate a node that contains the given uncompressed offset. It is
/// caller's job to check that target is not bigger than the uncompressed
/// size of the tree (the last node would be returned in that case still).
static void * index_tree_locate(const index_tree * tree, lzma_vli target)
{
	const index_tree_node * result = NULL;
	const index_tree_node * node = tree->root;
	assert(tree->leftmost == NULL || tree->leftmost->uncompressed_base == 0);
	// Consecutive nodes may have the same uncompressed_base.
	// We must pick the rightmost one.
	while(node != NULL) {
		if(node->uncompressed_base > target) {
			node = node->left;
		}
		else {
			result = node;
			node = node->right;
		}
	}
	return (void *)(result);
}

/// Allocate and initialize a new Stream using the given base offsets.
static index_stream * index_stream_init(lzma_vli compressed_base, lzma_vli uncompressed_base,
    uint32_t stream_number, lzma_vli block_number_base, const lzma_allocator * allocator)
{
	index_stream * s = static_cast<index_stream *>(lzma_alloc(sizeof(index_stream), allocator));
	if(s) {
		s->node.uncompressed_base = uncompressed_base;
		s->node.compressed_base = compressed_base;
		s->node.parent = NULL;
		s->node.left = NULL;
		s->node.right = NULL;
		s->number = stream_number;
		s->block_number_base = block_number_base;
		index_tree_init(&s->groups);
		s->record_count = 0;
		s->index_list_size = 0;
		s->stream_flags.version = UINT32_MAX;
		s->stream_padding = 0;
	}
	return s;
}

/// Free the memory allocated for a Stream and its Record groups.
static void index_stream_end(void * node, const lzma_allocator * allocator)
{
	index_stream * s = static_cast<index_stream *>(node);
	index_tree_end(&s->groups, allocator, &lzma_free);
	lzma_free(s, allocator);
}

static lzma_index * index_init_plain(const lzma_allocator * allocator)
{
	lzma_index * i = static_cast<lzma_index *>(lzma_alloc(sizeof(lzma_index), allocator));
	if(i) {
		index_tree_init(&i->streams);
		i->uncompressed_size = 0;
		i->total_size = 0;
		i->record_count = 0;
		i->index_list_size = 0;
		i->prealloc = INDEX_GROUP_SIZE;
		i->checks = 0;
	}
	return i;
}

lzma_index * lzma_index_init(const lzma_allocator *allocator)
{
	lzma_index * i = index_init_plain(allocator);
	if(i) {
		index_stream * s = index_stream_init(0, 0, 1, 0, allocator);
		if(s == NULL) {
			lzma_free(i, allocator);
			return NULL;
		}
		else
			index_tree_append(&i->streams, &s->node);
	}
	return i;
}

void lzma_index_end(lzma_index *i, const lzma_allocator *allocator)
{
	// NOTE: If you modify this function, check also the bottom
	// of lzma_index_cat().
	if(i) {
		index_tree_end(&i->streams, allocator, &index_stream_end);
		lzma_free(i, allocator);
	}
}

extern void lzma_index_prealloc(lzma_index * i, lzma_vli records)
{
	if(records > PREALLOC_MAX)
		records = PREALLOC_MAX;
	i->prealloc = (size_t)(records);
}

uint64_t lzma_index_memusage(lzma_vli streams, lzma_vli blocks)
{
	// This calculates an upper bound that is only a little bit
	// bigger than the exact maximum memory usage with the given
	// parameters.

	// Typical malloc() overhead is 2 * sizeof(void *) but we take
	// a little bit extra just in case. Using LZMA_MEMUSAGE_BASE
	// instead would give too inaccurate estimate.
	const size_t alloc_overhead = 4 * sizeof(void *);
	// Amount of memory needed for each Stream base structures.
	// We assume that every Stream has at least one Block and
	// thus at least one group.
	const size_t stream_base = sizeof(index_stream) + sizeof(index_group) + 2 * alloc_overhead;
	// Amount of memory needed per group.
	const size_t group_base = sizeof(index_group) + INDEX_GROUP_SIZE * sizeof(index_record) + alloc_overhead;
	// Number of groups. There may actually be more, but that overhead
	// has been taken into account in stream_base already.
	const lzma_vli groups = (blocks + INDEX_GROUP_SIZE - 1) / INDEX_GROUP_SIZE;
	// Memory used by index_stream and index_group structures.
	const uint64_t streams_mem = streams * stream_base;
	const uint64_t groups_mem = groups * group_base;
	// Memory used by the base structure.
	const uint64_t index_base = sizeof(lzma_index) + alloc_overhead;
	// Validate the arguments and catch integer overflows.
	// Maximum number of Streams is "only" UINT32_MAX, because
	// that limit is used by the tree containing the Streams.
	const uint64_t limit = UINT64_MAX - index_base;
	if(streams == 0 || streams > UINT32_MAX || blocks > LZMA_VLI_MAX || streams > limit / stream_base || groups > limit / group_base || limit - streams_mem < groups_mem)
		return UINT64_MAX;
	return index_base + streams_mem + groups_mem;
}

uint64_t lzma_index_memused(const lzma_index *i) { return lzma_index_memusage(i->streams.count, i->record_count); }
lzma_vli lzma_index_block_count(const lzma_index *i) { return i->record_count; }
lzma_vli lzma_index_stream_count(const lzma_index *i) { return i->streams.count; }
lzma_vli lzma_index_size(const lzma_index *i) { return index_size(i->record_count, i->index_list_size); }
lzma_vli lzma_index_total_size(const lzma_index *i) { return i->total_size; }

lzma_vli lzma_index_stream_size(const lzma_index *i)
{
	// Stream Header + Blocks + Index + Stream Footer
	return LZMA_STREAM_HEADER_SIZE + i->total_size + index_size(i->record_count, i->index_list_size) + LZMA_STREAM_HEADER_SIZE;
}

static lzma_vli index_file_size(lzma_vli compressed_base, lzma_vli unpadded_sum, lzma_vli record_count, lzma_vli index_list_size, lzma_vli stream_padding)
{
	// Earlier Streams and Stream Paddings + Stream Header
	// + Blocks + Index + Stream Footer + Stream Padding
	//
	// This might go over LZMA_VLI_MAX due to too big unpadded_sum
	// when this function is used in lzma_index_append().
	lzma_vli file_size = compressed_base + 2 * LZMA_STREAM_HEADER_SIZE + stream_padding + vli_ceil4(unpadded_sum);
	if(file_size > LZMA_VLI_MAX)
		return LZMA_VLI_UNKNOWN;
	// The same applies here.
	file_size += index_size(record_count, index_list_size);
	if(file_size > LZMA_VLI_MAX)
		return LZMA_VLI_UNKNOWN;
	return file_size;
}

lzma_vli lzma_index_file_size(const lzma_index *i)
{
	const index_stream * s = (const index_stream*)(i->streams.rightmost);
	const index_group * g = (const index_group*)(s->groups.rightmost);
	return index_file_size(s->node.compressed_base, g == NULL ? 0 : g->records[g->last].unpadded_sum, s->record_count, s->index_list_size, s->stream_padding);
}

lzma_vli lzma_index_uncompressed_size(const lzma_index *i)
{
	return i->uncompressed_size;
}

uint32_t lzma_index_checks(const lzma_index *i)
{
	uint32_t checks = i->checks;
	// Get the type of the Check of the last Stream too.
	const index_stream * s = (const index_stream*)(i->streams.rightmost);
	if(s->stream_flags.version != UINT32_MAX)
		checks |= UINT32_C(1) << s->stream_flags.check;
	return checks;
}

extern uint32_t lzma_index_padding_size(const lzma_index * i)
{
	return static_cast<uint32_t>((LZMA_VLI_C(4) - index_size_unpadded(i->record_count, i->index_list_size)) & 3);
}

lzma_ret lzma_index_stream_flags(lzma_index *i, const lzma_stream_flags *stream_flags)
{
	if(i == NULL || stream_flags == NULL)
		return LZMA_PROG_ERROR;
	else {
		// Validate the Stream Flags.
		return_if_error(lzma_stream_flags_compare(stream_flags, stream_flags));
		index_stream * s = (index_stream*)(i->streams.rightmost);
		s->stream_flags = *stream_flags;
		return LZMA_OK;
	}
}

lzma_ret lzma_index_stream_padding(lzma_index *i, lzma_vli stream_padding)
{
	if(i == NULL || stream_padding > LZMA_VLI_MAX || (stream_padding & 3) != 0)
		return LZMA_PROG_ERROR;
	index_stream * s = (index_stream*)(i->streams.rightmost);
	// Check that the new value won't make the file grow too big.
	const lzma_vli old_stream_padding = s->stream_padding;
	s->stream_padding = 0;
	if(lzma_index_file_size(i) + stream_padding > LZMA_VLI_MAX) {
		s->stream_padding = old_stream_padding;
		return LZMA_DATA_ERROR;
	}

	s->stream_padding = stream_padding;
	return LZMA_OK;
}

lzma_ret lzma_index_append(lzma_index *i, const lzma_allocator *allocator, lzma_vli unpadded_size, lzma_vli uncompressed_size)
{
	// Validate.
	if(i == NULL || unpadded_size < UNPADDED_SIZE_MIN || unpadded_size > UNPADDED_SIZE_MAX || uncompressed_size > LZMA_VLI_MAX)
		return LZMA_PROG_ERROR;
	index_stream * s = (index_stream*)(i->streams.rightmost);
	index_group * g = (index_group*)(s->groups.rightmost);
	const lzma_vli compressed_base = g == NULL ? 0 : vli_ceil4(g->records[g->last].unpadded_sum);
	const lzma_vli uncompressed_base = g == NULL ? 0 : g->records[g->last].uncompressed_sum;
	const uint32_t index_list_size_add = lzma_vli_size(unpadded_size) + lzma_vli_size(uncompressed_size);
	// Check that the file size will stay within limits.
	if(index_file_size(s->node.compressed_base,
	    compressed_base + unpadded_size, s->record_count + 1,
	    s->index_list_size + index_list_size_add,
	    s->stream_padding) == LZMA_VLI_UNKNOWN)
		return LZMA_DATA_ERROR;

	// The size of the Index field must not exceed the maximum value
	// that can be stored in the Backward Size field.
	if(index_size(i->record_count + 1, i->index_list_size + index_list_size_add) > LZMA_BACKWARD_SIZE_MAX)
		return LZMA_DATA_ERROR;

	if(g != NULL && g->last + 1 < g->allocated) {
		// There is space in the last group at least for one Record.
		++g->last;
	}
	else {
		// We need to allocate a new group.
		g = static_cast<index_group *>(lzma_alloc(sizeof(index_group) + i->prealloc * sizeof(index_record), allocator));
		if(g == NULL)
			return LZMA_MEM_ERROR;
		g->last = 0;
		g->allocated = i->prealloc;
		// Reset prealloc so that if the application happens to
		// add new Records, the allocation size will be sane.
		i->prealloc = INDEX_GROUP_SIZE;

		// Set the start offsets of this group.
		g->node.uncompressed_base = uncompressed_base;
		g->node.compressed_base = compressed_base;
		g->number_base = s->record_count + 1;

		// Add the new group to the Stream.
		index_tree_append(&s->groups, &g->node);
	}

	// Add the new Record to the group.
	g->records[g->last].uncompressed_sum
		= uncompressed_base + uncompressed_size;
	g->records[g->last].unpadded_sum
		= compressed_base + unpadded_size;

	// Update the totals.
	++s->record_count;
	s->index_list_size += index_list_size_add;

	i->total_size += vli_ceil4(unpadded_size);
	i->uncompressed_size += uncompressed_size;
	++i->record_count;
	i->index_list_size += index_list_size_add;
	return LZMA_OK;
}

/// Structure to pass info to index_cat_helper()
struct index_cat_info {
	index_cat_info(lzma_index * pIdx, lzma_vli destFileSize) : uncompressed_size(pIdx->uncompressed_size), file_size(destFileSize), 
		block_number_add(pIdx->record_count), stream_number_add(pIdx->streams.count), streams(&pIdx->streams)
	{
	}
		//.uncompressed_size = dest->uncompressed_size,
		//.file_size = dest_file_size,
		//.stream_number_add = dest->streams.count,
		//.block_number_add = dest->record_count,
		//.streams = &dest->streams,
	lzma_vli uncompressed_size; /// Uncompressed size of the destination
	lzma_vli file_size; /// Compressed file size of the destination
	lzma_vli block_number_add; /// Same as above but for Block numbers
	/// Number of Streams that were in the destination index before we
	/// started appending new Streams from the source index. This is used to fix the Stream numbering.
	uint32_t stream_number_add;
	index_tree * streams; /// Destination index' Stream tree
};

/// Add the Stream nodes from the source index to dest using recursion.
/// Simplest iterative traversal of the source tree wouldn't work, because
/// we update the pointers in nodes when moving them to the destination tree.
static void index_cat_helper(const index_cat_info * info, index_stream * pThis)
{
	index_stream * left = (index_stream*)(pThis->node.left);
	index_stream * right = (index_stream*)(pThis->node.right);
	if(left != NULL)
		index_cat_helper(info, left);
	pThis->node.uncompressed_base += info->uncompressed_size;
	pThis->node.compressed_base += info->file_size;
	pThis->number += info->stream_number_add;
	pThis->block_number_base += info->block_number_add;
	index_tree_append(info->streams, &pThis->node);
	if(right != NULL)
		index_cat_helper(info, right);
}

lzma_ret lzma_index_cat(lzma_index * dest, lzma_index * src, const lzma_allocator * allocator)
{
	const lzma_vli dest_file_size = lzma_index_file_size(dest);
	// Check that we don't exceed the file size limits.
	if(dest_file_size + lzma_index_file_size(src) > LZMA_VLI_MAX || dest->uncompressed_size + src->uncompressed_size > LZMA_VLI_MAX)
		return LZMA_DATA_ERROR;
	// Check that the encoded size of the combined lzma_indexes stays
	// within limits. In theory, this should be done only if we know
	// that the user plans to actually combine the Streams and thus
	// construct a single Index (probably rare). However, exceeding
	// this limit is quite theoretical, so we do this check always
	// to simplify things elsewhere.
	{
		const lzma_vli dest_size = index_size_unpadded(dest->record_count, dest->index_list_size);
		const lzma_vli src_size = index_size_unpadded(src->record_count, src->index_list_size);
		if(vli_ceil4(dest_size + src_size) > LZMA_BACKWARD_SIZE_MAX)
			return LZMA_DATA_ERROR;
	}

	// Optimize the last group to minimize memory usage. Allocation has
	// to be done before modifying dest or src.
	{
		index_stream * s = (index_stream*)(dest->streams.rightmost);
		index_group * g = (index_group*)(s->groups.rightmost);
		if(g != NULL && g->last + 1 < g->allocated) {
			assert(g->node.left == NULL);
			assert(g->node.right == NULL);
			index_group * newg = static_cast<index_group *>(lzma_alloc(sizeof(index_group) + (g->last + 1) * sizeof(index_record), allocator));
			if(newg == NULL)
				return LZMA_MEM_ERROR;
			newg->node = g->node;
			newg->allocated = g->last + 1;
			newg->last = g->last;
			newg->number_base = g->number_base;
			memcpy(newg->records, g->records, newg->allocated * sizeof(index_record));
			if(g->node.parent != NULL) {
				assert(g->node.parent->right == &g->node);
				g->node.parent->right = &newg->node;
			}
			if(s->groups.leftmost == &g->node) {
				assert(s->groups.root == &g->node);
				s->groups.leftmost = &newg->node;
				s->groups.root = &newg->node;
			}
			assert(s->groups.rightmost == &g->node);
			s->groups.rightmost = &newg->node;
			lzma_free(g, allocator);
			// NOTE: newg isn't leaked here because
			// newg == (void *)&newg->node.
		}
	}
	// Add all the Streams from src to dest. Update the base offsets of each Stream from src.
	/* @sobolev const index_cat_info info = {
		.uncompressed_size = dest->uncompressed_size,
		.file_size = dest_file_size,
		.stream_number_add = dest->streams.count,
		.block_number_add = dest->record_count,
		.streams = &dest->streams,
	};*/
	const index_cat_info info(dest, dest_file_size); // @sobolev
	index_cat_helper(&info, (index_stream*)(src->streams.root));

	// Update info about all the combined Streams.
	dest->uncompressed_size += src->uncompressed_size;
	dest->total_size += src->total_size;
	dest->record_count += src->record_count;
	dest->index_list_size += src->index_list_size;
	dest->checks = lzma_index_checks(dest) | src->checks;

	// There's nothing else left in src than the base structure.
	lzma_free(src, allocator);

	return LZMA_OK;
}

/// Duplicate an index_stream.
static index_stream * index_dup_stream(const index_stream * src, const lzma_allocator * allocator)
{
	// Catch a somewhat theoretical integer overflow.
	if(src->record_count > PREALLOC_MAX)
		return NULL;
	// Allocate and initialize a new Stream.
	index_stream * dest = index_stream_init(src->node.compressed_base, src->node.uncompressed_base, src->number, src->block_number_base, allocator);
	if(!dest)
		return NULL;
	// Copy the overall information.
	dest->record_count = src->record_count;
	dest->index_list_size = src->index_list_size;
	dest->stream_flags = src->stream_flags;
	dest->stream_padding = src->stream_padding;

	// Return if there are no groups to duplicate.
	if(src->groups.leftmost == NULL)
		return dest;

	// Allocate memory for the Records. We put all the Records into
	// a single group. It's simplest and also tends to make
	// lzma_index_locate() a little bit faster with very big Indexes.
	index_group * destg = static_cast<index_group *>(lzma_alloc(sizeof(index_group) + static_cast<size_t>(src->record_count) * sizeof(index_record), allocator));
	if(destg == NULL) {
		index_stream_end(dest, allocator);
		return NULL;
	}
	// Initialize destg.
	destg->node.uncompressed_base = 0;
	destg->node.compressed_base = 0;
	destg->number_base = 1;
	destg->allocated = static_cast<size_t>(src->record_count);
	destg->last = static_cast<size_t>(src->record_count - 1);
	// Go through all the groups in src and copy the Records into destg.
	const index_group * srcg = (const index_group*)(src->groups.leftmost);
	size_t i = 0;
	do {
		memcpy(destg->records + i, srcg->records, (srcg->last + 1) * sizeof(index_record));
		i += srcg->last + 1;
		srcg = static_cast<const index_group *>(index_tree_next(&srcg->node));
	} while(srcg != NULL);
	assert(i == destg->allocated);
	// Add the group to the new Stream.
	index_tree_append(&dest->groups, &destg->node);
	return dest;
}

lzma_index * lzma_index_dup(const lzma_index *src, const lzma_allocator *allocator)
{
	// Allocate the base structure (no initial Stream).
	lzma_index * dest = index_init_plain(allocator);
	if(!dest)
		return NULL;
	// Copy the totals.
	dest->uncompressed_size = src->uncompressed_size;
	dest->total_size = src->total_size;
	dest->record_count = src->record_count;
	dest->index_list_size = src->index_list_size;
	// Copy the Streams and the groups in them.
	const index_stream * srcstream = (const index_stream*)(src->streams.leftmost);
	do {
		index_stream * deststream = index_dup_stream(srcstream, allocator);
		if(deststream == NULL) {
			lzma_index_end(dest, allocator);
			return NULL;
		}
		index_tree_append(&dest->streams, &deststream->node);
		srcstream = static_cast<const index_stream *>(index_tree_next(&srcstream->node));
	} while(srcstream != NULL);
	return dest;
}

/// Indexing for lzma_index_iter.internal[]
enum {
	ITER_INDEX,
	ITER_STREAM,
	ITER_GROUP,
	ITER_RECORD,
	ITER_METHOD,
};

/// Values for lzma_index_iter.internal[ITER_METHOD].s
enum {
	ITER_METHOD_NORMAL,
	ITER_METHOD_NEXT,
	ITER_METHOD_LEFTMOST,
};

static void iter_set_info(lzma_index_iter * iter)
{
	const lzma_index * i = static_cast<const lzma_index *>(iter->internal[ITER_INDEX].p);
	const index_stream * stream = static_cast<const index_stream *>(iter->internal[ITER_STREAM].p);
	const index_group * group = static_cast<const index_group *>(iter->internal[ITER_GROUP].p);
	const size_t record = iter->internal[ITER_RECORD].s;

	// lzma_index_iter.internal must not contain a pointer to the last
	// group in the index, because that may be reallocated by
	// lzma_index_cat().
	if(group == NULL) {
		// There are no groups.
		assert(stream->groups.root == NULL);
		iter->internal[ITER_METHOD].s = ITER_METHOD_LEFTMOST;
	}
	else if(i->streams.rightmost != &stream->node
	   || stream->groups.rightmost != &group->node) {
		// The group is not not the last group in the index.
		iter->internal[ITER_METHOD].s = ITER_METHOD_NORMAL;
	}
	else if(stream->groups.leftmost != &group->node) {
		// The group isn't the only group in the Stream, thus we
		// know that it must have a parent group i.e. it's not
		// the root node.
		assert(stream->groups.root != &group->node);
		assert(group->node.parent->right == &group->node);
		iter->internal[ITER_METHOD].s = ITER_METHOD_NEXT;
		iter->internal[ITER_GROUP].p = group->node.parent;
	}
	else {
		// The Stream has only one group.
		assert(stream->groups.root == &group->node);
		assert(group->node.parent == NULL);
		iter->internal[ITER_METHOD].s = ITER_METHOD_LEFTMOST;
		iter->internal[ITER_GROUP].p = NULL;
	}

	// NOTE: lzma_index_iter.stream.number is lzma_vli but we use uint32_t
	// internally.
	iter->stream.number = stream->number;
	iter->stream.block_count = stream->record_count;
	iter->stream.compressed_offset = stream->node.compressed_base;
	iter->stream.uncompressed_offset = stream->node.uncompressed_base;
	// iter->stream.flags will be NULL if the Stream Flags haven't been
	// set with lzma_index_stream_flags().
	iter->stream.flags = stream->stream_flags.version == UINT32_MAX ? NULL : &stream->stream_flags;
	iter->stream.padding = stream->stream_padding;
	if(stream->groups.rightmost == NULL) {
		// Stream has no Blocks.
		iter->stream.compressed_size = index_size(0, 0) + 2 * LZMA_STREAM_HEADER_SIZE;
		iter->stream.uncompressed_size = 0;
	}
	else {
		const index_group * g = (const index_group*)(stream->groups.rightmost);
		// Stream Header + Stream Footer + Index + Blocks
		iter->stream.compressed_size = 2 * LZMA_STREAM_HEADER_SIZE + index_size(stream->record_count, stream->index_list_size) + vli_ceil4(g->records[g->last].unpadded_sum);
		iter->stream.uncompressed_size = g->records[g->last].uncompressed_sum;
	}

	if(group != NULL) {
		iter->block.number_in_stream = group->number_base + record;
		iter->block.number_in_file = iter->block.number_in_stream + stream->block_number_base;
		iter->block.compressed_stream_offset = record == 0 ? group->node.compressed_base : vli_ceil4(group->records[record - 1].unpadded_sum);
		iter->block.uncompressed_stream_offset = record == 0 ? group->node.uncompressed_base : group->records[record - 1].uncompressed_sum;
		iter->block.uncompressed_size = group->records[record].uncompressed_sum - iter->block.uncompressed_stream_offset;
		iter->block.unpadded_size = group->records[record].unpadded_sum - iter->block.compressed_stream_offset;
		iter->block.total_size = vli_ceil4(iter->block.unpadded_size);
		iter->block.compressed_stream_offset += LZMA_STREAM_HEADER_SIZE;
		iter->block.compressed_file_offset = iter->block.compressed_stream_offset + iter->stream.compressed_offset;
		iter->block.uncompressed_file_offset = iter->block.uncompressed_stream_offset + iter->stream.uncompressed_offset;
	}
}

void lzma_index_iter_init(lzma_index_iter *iter, const lzma_index *i)
{
	iter->internal[ITER_INDEX].p = i;
	lzma_index_iter_rewind(iter);
}

void lzma_index_iter_rewind(lzma_index_iter *iter)
{
	iter->internal[ITER_STREAM].p = NULL;
	iter->internal[ITER_GROUP].p = NULL;
	iter->internal[ITER_RECORD].s = 0;
	iter->internal[ITER_METHOD].s = ITER_METHOD_NORMAL;
}

bool lzma_index_iter_next(lzma_index_iter *iter, lzma_index_iter_mode mode)
{
	// Catch unsupported mode values.
	if((uint)(mode) > LZMA_INDEX_ITER_NONEMPTY_BLOCK)
		return true;
	const lzma_index * i = static_cast<const lzma_index *>(iter->internal[ITER_INDEX].p);
	const index_stream * stream = static_cast<const index_stream *>(iter->internal[ITER_STREAM].p);
	const index_group * group = NULL;
	size_t record = iter->internal[ITER_RECORD].s;

	// If we are being asked for the next Stream, leave group to NULL
	// so that the rest of the this function thinks that this Stream
	// has no groups and will thus go to the next Stream.
	if(mode != LZMA_INDEX_ITER_STREAM) {
		// Get the pointer to the current group. See iter_set_inf()
		// for explanation.
		switch(iter->internal[ITER_METHOD].s) {
			case ITER_METHOD_NORMAL:
			    group = static_cast<const index_group *>(iter->internal[ITER_GROUP].p);
			    break;
			case ITER_METHOD_NEXT:
			    group = static_cast<const index_group *>(index_tree_next(static_cast<const index_tree_node *>(iter->internal[ITER_GROUP].p)));
			    break;
			case ITER_METHOD_LEFTMOST:
			    group = (const index_group*)(stream->groups.leftmost);
			    break;
		}
	}

again:
	if(stream == NULL) {
		// We at the beginning of the lzma_index.
		// Locate the first Stream.
		stream = (const index_stream*)(i->streams.leftmost);
		if(mode >= LZMA_INDEX_ITER_BLOCK) {
			// Since we are being asked to return information
			// about the first a Block, skip Streams that have
			// no Blocks.
			while(stream->groups.leftmost == NULL) {
				stream = static_cast<const index_stream *>(index_tree_next(&stream->node));
				if(stream == NULL)
					return true;
			}
		}
		// Start from the first Record in the Stream.
		group = (const index_group*)(stream->groups.leftmost);
		record = 0;
	}
	else if(group != NULL && record < group->last) {
		// The next Record is in the same group.
		++record;
	}
	else {
		// This group has no more Records or this Stream has
		// no Blocks at all.
		record = 0;

		// If group is not NULL, this Stream has at least one Block
		// and thus at least one group. Find the next group.
		if(group != NULL)
			group = static_cast<const index_group *>(index_tree_next(&group->node));
		if(group == NULL) {
			// This Stream has no more Records. Find the next
			// Stream. If we are being asked to return information
			// about a Block, we skip empty Streams.
			do {
				stream = static_cast<const index_stream *>(index_tree_next(&stream->node));
				if(stream == NULL)
					return true;
			} while(mode >= LZMA_INDEX_ITER_BLOCK && stream->groups.leftmost == NULL);
			group = (const index_group*)(stream->groups.leftmost);
		}
	}
	if(mode == LZMA_INDEX_ITER_NONEMPTY_BLOCK) {
		// We need to look for the next Block again if this Block
		// is empty.
		if(record == 0) {
			if(group->node.uncompressed_base == group->records[0].uncompressed_sum)
				goto again;
		}
		else if(group->records[record - 1].uncompressed_sum == group->records[record].uncompressed_sum) {
			goto again;
		}
	}

	iter->internal[ITER_STREAM].p = stream;
	iter->internal[ITER_GROUP].p = group;
	iter->internal[ITER_RECORD].s = record;

	iter_set_info(iter);

	return false;
}

bool lzma_index_iter_locate(lzma_index_iter *iter, lzma_vli target)
{
	const lzma_index * i = static_cast<const lzma_index *>(iter->internal[ITER_INDEX].p);
	// If the target is past the end of the file, return immediately.
	if(i->uncompressed_size <= target)
		return true;
	// Locate the Stream containing the target offset.
	const index_stream * stream = static_cast<const index_stream *>(index_tree_locate(&i->streams, target));
	assert(stream != NULL);
	target -= stream->node.uncompressed_base;
	// Locate the group containing the target offset.
	const index_group * group = static_cast<const index_group *>(index_tree_locate(&stream->groups, target));
	assert(group != NULL);
	// Use binary search to locate the exact Record. It is the first
	// Record whose uncompressed_sum is greater than target.
	// This is because we want the rightmost Record that fulfills the
	// search criterion. It is possible that there are empty Blocks;
	// we don't want to return them.
	size_t left = 0;
	size_t right = group->last;
	while(left < right) {
		const size_t pos = left + (right - left) / 2;
		if(group->records[pos].uncompressed_sum <= target)
			left = pos + 1;
		else
			right = pos;
	}
	iter->internal[ITER_STREAM].p = stream;
	iter->internal[ITER_GROUP].p = group;
	iter->internal[ITER_RECORD].s = left;
	iter_set_info(iter);
	return false;
}
//
// index_hash
//
struct lzma_index_hash_info {
	lzma_vli blocks_size; /// Sum of the Block sizes (including Block Padding)
	lzma_vli uncompressed_size; /// Sum of the Uncompressed Size fields
	lzma_vli count; /// Number of Records
	lzma_vli index_list_size; /// Size of the List of Index Records as bytes
	lzma_check_state check; /// Check calculated from Unpadded Sizes and Uncompressed Sizes.
};

struct lzma_index_hash_s {
	enum {
		SEQ_BLOCK,
		SEQ_COUNT,
		SEQ_UNPADDED,
		SEQ_UNCOMPRESSED,
		SEQ_PADDING_INIT,
		SEQ_PADDING,
		SEQ_CRC32,
	} sequence;

	lzma_index_hash_info blocks; /// Information collected while decoding the actual Blocks.
	lzma_index_hash_info records; /// Information collected from the Index field.
	lzma_vli remaining; /// Number of Records not fully decoded
	lzma_vli unpadded_size; /// Unpadded Size currently being read from an Index Record.
	lzma_vli uncompressed_size; /// Uncompressed Size currently being read from an Index Record.
	size_t pos; /// Position in variable-length integers when decoding them from the List of Records.
	uint32_t crc32; /// CRC32 of the Index
};

lzma_index_hash * lzma_index_hash_init(lzma_index_hash * index_hash, const lzma_allocator *allocator)
{
	SETIFZ(index_hash, static_cast<lzma_index_hash *>(lzma_alloc(sizeof(lzma_index_hash), allocator)));
	if(index_hash) {
		index_hash->sequence = lzma_index_hash_s::SEQ_BLOCK;
		index_hash->blocks.blocks_size = 0;
		index_hash->blocks.uncompressed_size = 0;
		index_hash->blocks.count = 0;
		index_hash->blocks.index_list_size = 0;
		index_hash->records.blocks_size = 0;
		index_hash->records.uncompressed_size = 0;
		index_hash->records.count = 0;
		index_hash->records.index_list_size = 0;
		index_hash->unpadded_size = 0;
		index_hash->uncompressed_size = 0;
		index_hash->pos = 0;
		index_hash->crc32 = 0;
		// These cannot fail because LZMA_CHECK_BEST is known to be supported.
		lzma_check_init(&index_hash->blocks.check, LZMA_CHECK_BEST);
		lzma_check_init(&index_hash->records.check, LZMA_CHECK_BEST);
	}
	return index_hash;
}

void lzma_index_hash_end(lzma_index_hash *index_hash, const lzma_allocator *allocator)
{
	lzma_free(index_hash, allocator);
}

lzma_vli lzma_index_hash_size(const lzma_index_hash *index_hash)
{
	// Get the size of the Index from ->blocks instead of ->records for
	// cases where application wants to know the Index Size before
	// decoding the Index.
	return index_size(index_hash->blocks.count, index_hash->blocks.index_list_size);
}

/// Updates the sizes and the hash without any validation.
static lzma_ret hash_append(lzma_index_hash_info * info, lzma_vli unpadded_size, lzma_vli uncompressed_size)
{
	info->blocks_size += vli_ceil4(unpadded_size);
	info->uncompressed_size += uncompressed_size;
	info->index_list_size += lzma_vli_size(unpadded_size) + lzma_vli_size(uncompressed_size);
	++info->count;
	const lzma_vli sizes[2] = { unpadded_size, uncompressed_size };
	lzma_check_update(&info->check, LZMA_CHECK_BEST, (const uint8_t*)(sizes), sizeof(sizes));
	return LZMA_OK;
}

lzma_ret lzma_index_hash_append(lzma_index_hash *index_hash, lzma_vli unpadded_size, lzma_vli uncompressed_size)
{
	// Validate the arguments.
	if(index_hash->sequence != lzma_index_hash_s::SEQ_BLOCK || unpadded_size < UNPADDED_SIZE_MIN || unpadded_size > UNPADDED_SIZE_MAX || uncompressed_size > LZMA_VLI_MAX)
		return LZMA_PROG_ERROR;
	// Update the hash.
	return_if_error(hash_append(&index_hash->blocks, unpadded_size, uncompressed_size));
	// Validate the properties of *info are still in allowed limits.
	if(index_hash->blocks.blocks_size > LZMA_VLI_MAX || index_hash->blocks.uncompressed_size > LZMA_VLI_MAX
	   || index_size(index_hash->blocks.count, index_hash->blocks.index_list_size) > LZMA_BACKWARD_SIZE_MAX
	   || index_stream_size(index_hash->blocks.blocks_size, index_hash->blocks.count, index_hash->blocks.index_list_size) > LZMA_VLI_MAX)
		return LZMA_DATA_ERROR;
	return LZMA_OK;
}

lzma_ret lzma_index_hash_decode(lzma_index_hash *index_hash, const uint8_t *in, size_t *in_pos, size_t in_size)
{
	// Catch zero input buffer here, because in contrast to Index encoder
	// and decoder functions, applications call this function directly
	// instead of via lzma_code(), which does the buffer checking.
	if(*in_pos >= in_size)
		return LZMA_BUF_ERROR;
	// NOTE: This function has many similarities to index_encode() and
	// index_decode() functions found from index_encoder.c and
	// index_decoder.c. See the comments especially in index_encoder.c.
	const size_t in_start = *in_pos;
	lzma_ret ret = LZMA_OK;
	while(*in_pos < in_size)
		switch(index_hash->sequence) {
			case lzma_index_hash_s::SEQ_BLOCK:
			    // Check the Index Indicator is present.
			    if(in[(*in_pos)++] != 0x00)
				    return LZMA_DATA_ERROR;
			    index_hash->sequence = lzma_index_hash_s::SEQ_COUNT;
			    break;
			case lzma_index_hash_s::SEQ_COUNT: {
			    ret = lzma_vli_decode(&index_hash->remaining, &index_hash->pos, in, in_pos, in_size);
			    if(ret != LZMA_STREAM_END)
				    goto out;
			    // The count must match the count of the Blocks decoded.
			    if(index_hash->remaining != index_hash->blocks.count)
				    return LZMA_DATA_ERROR;
			    ret = LZMA_OK;
			    index_hash->pos = 0;
			    // Handle the special case when there are no Blocks.
			    index_hash->sequence = index_hash->remaining == 0 ? lzma_index_hash_s::SEQ_PADDING_INIT : lzma_index_hash_s::SEQ_UNPADDED;
			    break;
		    }
			case lzma_index_hash_s::SEQ_UNPADDED:
			case lzma_index_hash_s::SEQ_UNCOMPRESSED: {
			    lzma_vli * size = index_hash->sequence == lzma_index_hash_s::SEQ_UNPADDED ? &index_hash->unpadded_size : &index_hash->uncompressed_size;
			    ret = lzma_vli_decode(size, &index_hash->pos, in, in_pos, in_size);
			    if(ret != LZMA_STREAM_END)
				    goto out;
			    ret = LZMA_OK;
			    index_hash->pos = 0;
			    if(index_hash->sequence == lzma_index_hash_s::SEQ_UNPADDED) {
				    if(index_hash->unpadded_size < UNPADDED_SIZE_MIN || index_hash->unpadded_size > UNPADDED_SIZE_MAX)
					    return LZMA_DATA_ERROR;
				    index_hash->sequence = lzma_index_hash_s::SEQ_UNCOMPRESSED;
			    }
			    else {
				    // Update the hash.
				    return_if_error(hash_append(&index_hash->records,
					index_hash->unpadded_size,
					index_hash->uncompressed_size));
				    // Verify that we don't go over the known sizes. Note
				    // that this validation is simpler than the one used
				    // in lzma_index_hash_append(), because here we know
				    // that values in index_hash->blocks are already
				    // validated and we are fine as long as we don't
				    // exceed them in index_hash->records.
				    if(index_hash->blocks.blocks_size < index_hash->records.blocks_size || index_hash->blocks.uncompressed_size < index_hash->records.uncompressed_size
					|| index_hash->blocks.index_list_size < index_hash->records.index_list_size)
					    return LZMA_DATA_ERROR;

				    // Check if this was the last Record.
				    index_hash->sequence = --index_hash->remaining == 0 ? lzma_index_hash_s::SEQ_PADDING_INIT : lzma_index_hash_s::SEQ_UNPADDED;
			    }

			    break;
		    }
			case lzma_index_hash_s::SEQ_PADDING_INIT:
			    index_hash->pos = (LZMA_VLI_C(4) - index_size_unpadded(index_hash->records.count, index_hash->records.index_list_size)) & 3;
			    index_hash->sequence = lzma_index_hash_s::SEQ_PADDING;
			// @fallthrough
			case lzma_index_hash_s::SEQ_PADDING:
			    if(index_hash->pos > 0) {
				    --index_hash->pos;
				    if(in[(*in_pos)++] != 0x00)
					    return LZMA_DATA_ERROR;
				    break;
			    }
			    // Compare the sizes.
			    if(index_hash->blocks.blocks_size != index_hash->records.blocks_size || 
					index_hash->blocks.uncompressed_size != index_hash->records.uncompressed_size || index_hash->blocks.index_list_size != index_hash->records.index_list_size)
				    return LZMA_DATA_ERROR;

			    // Finish the hashes and compare them.
			    lzma_check_finish(&index_hash->blocks.check, LZMA_CHECK_BEST);
			    lzma_check_finish(&index_hash->records.check, LZMA_CHECK_BEST);
			    if(memcmp(index_hash->blocks.check.buffer.u8, index_hash->records.check.buffer.u8, lzma_check_size(LZMA_CHECK_BEST)) != 0)
				    return LZMA_DATA_ERROR;
			    // Finish the CRC32 calculation.
			    index_hash->crc32 = lzma_crc32(in + in_start, *in_pos - in_start, index_hash->crc32);
			    index_hash->sequence = lzma_index_hash_s::SEQ_CRC32;
			// @fallthrough
			case lzma_index_hash_s::SEQ_CRC32:
			    do {
				    if(*in_pos == in_size)
					    return LZMA_OK;
				    if(((index_hash->crc32 >> (index_hash->pos * 8)) & 0xFF) != in[(*in_pos)++]) {
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
					    return LZMA_DATA_ERROR;
#endif
				    }
			    } while(++index_hash->pos < 4);
			    return LZMA_STREAM_END;
			default:
			    assert(0);
			    return LZMA_PROG_ERROR;
		}
out:
	// Update the CRC32,
	index_hash->crc32 = lzma_crc32(in + in_start, *in_pos - in_start, index_hash->crc32);
	return ret;
}
//
// index_encoder
//
static lzma_ret index_encode(void * coder_ptr,
    const lzma_allocator * allocator lzma_attribute((__unused__)),
    const uint8_t * in lzma_attribute((__unused__)),
    size_t * in_pos lzma_attribute((__unused__)),
    size_t in_size lzma_attribute((__unused__)),
    uint8_t * out, size_t * out_pos,
    size_t out_size,
    lzma_action action lzma_attribute((__unused__)))
{
	lzma_index_encoder_coder * coder = (lzma_index_encoder_coder *)coder_ptr;
	// Position where to start calculating CRC32. The idea is that we
	// need to call lzma_crc32() only once per call to index_encode().
	const size_t out_start = *out_pos;
	// Return value to use if we return at the end of this function.
	// We use "goto out" to jump out of the while-switch construct
	// instead of returning directly, because that way we don't need
	// to copypaste the lzma_crc32() call to many places.
	lzma_ret ret = LZMA_OK;
	while(*out_pos < out_size)
		switch(coder->sequence) {
			case lzma_index_encoder_coder::SEQ_INDICATOR:
			    out[*out_pos] = 0x00;
			    ++*out_pos;
			    coder->sequence = lzma_index_encoder_coder::SEQ_COUNT;
			    break;

			case lzma_index_encoder_coder::SEQ_COUNT: {
			    const lzma_vli count = lzma_index_block_count(coder->index);
			    ret = lzma_vli_encode(count, &coder->pos, out, out_pos, out_size);
			    if(ret != LZMA_STREAM_END)
				    goto out;

			    ret = LZMA_OK;
			    coder->pos = 0;
			    coder->sequence = lzma_index_encoder_coder::SEQ_NEXT;
			    break;
		    }
			case lzma_index_encoder_coder::SEQ_NEXT:
			    if(lzma_index_iter_next(&coder->iter, LZMA_INDEX_ITER_BLOCK)) {
				    // Get the size of the Index Padding field.
				    coder->pos = lzma_index_padding_size(coder->index);
				    assert(coder->pos <= 3);
				    coder->sequence = lzma_index_encoder_coder::SEQ_PADDING;
				    break;
			    }
			    coder->sequence = lzma_index_encoder_coder::SEQ_UNPADDED;
			// @fallthrough
			case lzma_index_encoder_coder::SEQ_UNPADDED:
			case lzma_index_encoder_coder::SEQ_UNCOMPRESSED: {
			    const lzma_vli size = coder->sequence == lzma_index_encoder_coder::SEQ_UNPADDED ? coder->iter.block.unpadded_size : coder->iter.block.uncompressed_size;
			    ret = lzma_vli_encode(size, &coder->pos, out, out_pos, out_size);
			    if(ret != LZMA_STREAM_END)
				    goto out;
			    ret = LZMA_OK;
			    coder->pos = 0;
			    // Advance to SEQ_UNCOMPRESSED or SEQ_NEXT.
			    coder->sequence = static_cast<lzma_index_encoder_coder::_Seq>(static_cast<int>(coder->sequence) + 1);
			    break;
		    }
			case lzma_index_encoder_coder::SEQ_PADDING:
			    if(coder->pos > 0) {
				    --coder->pos;
				    out[(*out_pos)++] = 0x00;
				    break;
			    }
			    // Finish the CRC32 calculation.
			    coder->crc32 = lzma_crc32(out + out_start, *out_pos - out_start, coder->crc32);
			    coder->sequence = lzma_index_encoder_coder::SEQ_CRC32;

			// @fallthrough
			case lzma_index_encoder_coder::SEQ_CRC32:
			    // We don't use the main loop, because we don't want
			    // coder->crc32 to be touched anymore.
			    do {
				    if(*out_pos == out_size)
					    return LZMA_OK;

				    out[*out_pos] = (coder->crc32 >> (coder->pos * 8))
					& 0xFF;
				    ++*out_pos;
			    } while(++coder->pos < 4);

			    return LZMA_STREAM_END;

			default:
			    assert(0);
			    return LZMA_PROG_ERROR;
		}

out:
	// Update the CRC32.
	coder->crc32 = lzma_crc32(out + out_start,
		*out_pos - out_start, coder->crc32);

	return ret;
}

static void index_encoder_end(void * coder, const lzma_allocator * allocator)
{
	lzma_free(coder, allocator);
}

static void index_encoder_reset(lzma_index_encoder_coder * coder, const lzma_index * i)
{
	lzma_index_iter_init(&coder->iter, i);
	coder->sequence = lzma_index_encoder_coder::SEQ_INDICATOR;
	coder->index = i;
	coder->pos = 0;
	coder->crc32 = 0;
}

extern lzma_ret lzma_index_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_index * i)
{
	lzma_next_coder_init(&lzma_index_encoder_init, next, allocator);
	if(i == NULL)
		return LZMA_PROG_ERROR;
	if(next->coder == NULL) {
		next->coder = lzma_alloc(sizeof(lzma_index_encoder_coder), allocator);
		if(next->coder == NULL)
			return LZMA_MEM_ERROR;
		next->code = &index_encode;
		next->end = &index_encoder_end;
	}
	index_encoder_reset((lzma_index_encoder_coder *)next->coder, i);
	return LZMA_OK;
}

lzma_ret lzma_index_encoder(lzma_stream *strm, const lzma_index *i)
{
	lzma_next_strm_init(lzma_index_encoder_init, strm, i);
	strm->internal->supported_actions[LZMA_RUN] = true;
	strm->internal->supported_actions[LZMA_FINISH] = true;
	return LZMA_OK;
}

lzma_ret lzma_index_buffer_encode(const lzma_index *i, uint8_t *out, size_t *out_pos, size_t out_size)
{
	// Validate the arguments.
	if(!i || !out || !out_pos || *out_pos > out_size)
		return LZMA_PROG_ERROR;
	// Don't try to encode if there's not enough output space.
	if(out_size - *out_pos < lzma_index_size(i))
		return LZMA_BUF_ERROR;
	// The Index encoder needs just one small data structure so we can
	// allocate it on stack.
	lzma_index_encoder_coder coder;
	index_encoder_reset(&coder, i);
	// Do the actual encoding. This should never fail, but store
	// the original *out_pos just in case.
	const size_t out_start = *out_pos;
	lzma_ret ret = index_encode(&coder, NULL, NULL, NULL, 0, out, out_pos, out_size, LZMA_RUN);
	if(ret == LZMA_STREAM_END) {
		ret = LZMA_OK;
	}
	else {
		// We should never get here, but just in case, restore the
		// output position and set the error accordingly if something
		// goes wrong and debugging isn't enabled.
		assert(0);
		*out_pos = out_start;
		ret = LZMA_PROG_ERROR;
	}
	return ret;
}
//
// index_decoder
//
static lzma_ret index_decode(void * coder_ptr, const lzma_allocator * allocator, const uint8_t * in, size_t * in_pos,
    size_t in_size, uint8_t * out lzma_attribute((__unused__)),
    size_t * out_pos lzma_attribute((__unused__)), size_t out_size lzma_attribute((__unused__)),
    lzma_action action lzma_attribute((__unused__)))
{
	lzma_index_decoder_coder * coder = (lzma_index_decoder_coder *)coder_ptr;
	// Similar optimization as in index_encoder.c
	const size_t in_start = *in_pos;
	lzma_ret ret = LZMA_OK;
	while(*in_pos < in_size)
		switch(coder->sequence) {
			case lzma_index_decoder_coder::SEQ_INDICATOR:
			    // Return LZMA_DATA_ERROR instead of e.g. LZMA_PROG_ERROR or
			    // LZMA_FORMAT_ERROR, because a typical usage case for Index
			    // decoder is when parsing the Stream backwards. If seeking
			    // backward from the Stream Footer gives us something that
			    // doesn't begin with Index Indicator, the file is considered
			    // corrupt, not "programming error" or "unrecognized file
			    // format". One could argue that the application should
			    // verify the Index Indicator before trying to decode the
			    // Index, but well, I suppose it is simpler this way.
			    if(in[(*in_pos)++] != 0x00)
				    return LZMA_DATA_ERROR;
			    coder->sequence = lzma_index_decoder_coder::SEQ_COUNT;
			    break;
			case lzma_index_decoder_coder::SEQ_COUNT:
			    ret = lzma_vli_decode(&coder->count, &coder->pos, in, in_pos, in_size);
			    if(ret != LZMA_STREAM_END)
				    goto out;
			    coder->pos = 0;
			    coder->sequence = lzma_index_decoder_coder::SEQ_MEMUSAGE;
			// @fallthrough
			case lzma_index_decoder_coder::SEQ_MEMUSAGE:
			    if(lzma_index_memusage(1, coder->count) > coder->memlimit) {
				    ret = LZMA_MEMLIMIT_ERROR;
				    goto out;
			    }
			    // Tell the Index handling code how many Records this
			    // Index has to allow it to allocate memory more efficiently.
			    lzma_index_prealloc(coder->index, coder->count);
			    ret = LZMA_OK;
			    coder->sequence = coder->count == 0 ? lzma_index_decoder_coder::SEQ_PADDING_INIT : lzma_index_decoder_coder::SEQ_UNPADDED;
			    break;
			case lzma_index_decoder_coder::SEQ_UNPADDED:
			case lzma_index_decoder_coder::SEQ_UNCOMPRESSED: {
			    lzma_vli * size = (coder->sequence == lzma_index_decoder_coder::SEQ_UNPADDED) ? &coder->unpadded_size : &coder->uncompressed_size;
			    ret = lzma_vli_decode(size, &coder->pos, in, in_pos, in_size);
			    if(ret != LZMA_STREAM_END)
				    goto out;
			    ret = LZMA_OK;
			    coder->pos = 0;
			    if(coder->sequence == lzma_index_decoder_coder::SEQ_UNPADDED) {
				    // Validate that encoded Unpadded Size isn't too small
				    // or too big.
				    if(coder->unpadded_size < UNPADDED_SIZE_MIN || coder->unpadded_size > UNPADDED_SIZE_MAX)
					    return LZMA_DATA_ERROR;
				    coder->sequence = lzma_index_decoder_coder::SEQ_UNCOMPRESSED;
			    }
			    else {
				    // Add the decoded Record to the Index.
				    return_if_error(lzma_index_append(coder->index, allocator, coder->unpadded_size, coder->uncompressed_size));
				    // Check if this was the last Record.
				    coder->sequence = (--coder->count == 0) ? lzma_index_decoder_coder::SEQ_PADDING_INIT : lzma_index_decoder_coder::SEQ_UNPADDED;
			    }
			    break;
		    }
			case lzma_index_decoder_coder::SEQ_PADDING_INIT:
			    coder->pos = lzma_index_padding_size(coder->index);
			    coder->sequence = lzma_index_decoder_coder::SEQ_PADDING;
			// @fallthrough
			case lzma_index_decoder_coder::SEQ_PADDING:
			    if(coder->pos > 0) {
				    --coder->pos;
				    if(in[(*in_pos)++] != 0x00)
					    return LZMA_DATA_ERROR;
				    break;
			    }
			    // Finish the CRC32 calculation.
			    coder->crc32 = lzma_crc32(in + in_start, *in_pos - in_start, coder->crc32);
			    coder->sequence = lzma_index_decoder_coder::SEQ_CRC32;
			// @fallthrough
			case lzma_index_decoder_coder::SEQ_CRC32:
			    do {
				    if(*in_pos == in_size)
					    return LZMA_OK;
				    if(((coder->crc32 >> (coder->pos * 8)) & 0xFF) != in[(*in_pos)++]) {
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
					    return LZMA_DATA_ERROR;
#endif
				    }
			    } while(++coder->pos < 4);
			    // Decoding was successful, now we can let the application
			    // see the decoded Index.
			    *coder->index_ptr = coder->index;
			    // Make index NULL so we don't free it unintentionally.
			    coder->index = NULL;
			    return LZMA_STREAM_END;
			default:
			    assert(0);
			    return LZMA_PROG_ERROR;
		}
out:
	// Update the CRC32,
	coder->crc32 = lzma_crc32(in + in_start, *in_pos - in_start, coder->crc32);
	return ret;
}

static void index_decoder_end(void * coder_ptr, const lzma_allocator * allocator)
{
	lzma_index_decoder_coder * coder = (lzma_index_decoder_coder *)coder_ptr;
	lzma_index_end(coder->index, allocator);
	lzma_free(coder, allocator);
}

static lzma_ret index_decoder_memconfig(void * coder_ptr, uint64_t * memusage, uint64_t * old_memlimit, uint64_t new_memlimit)
{
	lzma_index_decoder_coder * coder = (lzma_index_decoder_coder *)coder_ptr;
	*memusage = lzma_index_memusage(1, coder->count);
	*old_memlimit = coder->memlimit;
	if(new_memlimit != 0) {
		if(new_memlimit < *memusage)
			return LZMA_MEMLIMIT_ERROR;
		coder->memlimit = new_memlimit;
	}
	return LZMA_OK;
}

static lzma_ret index_decoder_reset(lzma_index_decoder_coder * coder, const lzma_allocator * allocator, lzma_index ** i, uint64_t memlimit)
{
	// Remember the pointer given by the application. We will set it
	// to point to the decoded Index only if decoding is successful.
	// Before that, keep it NULL so that applications can always safely
	// pass it to lzma_index_end() no matter did decoding succeed or not.
	coder->index_ptr = i;
	*i = NULL;
	// We always allocate a new lzma_index.
	coder->index = lzma_index_init(allocator);
	if(coder->index == NULL)
		return LZMA_MEM_ERROR;
	// Initialize the rest.
	coder->sequence = lzma_index_decoder_coder::SEQ_INDICATOR;
	coder->memlimit = MAX(1, memlimit);
	coder->count = 0; // Needs to be initialized due to _memconfig().
	coder->pos = 0;
	coder->crc32 = 0;
	return LZMA_OK;
}

extern lzma_ret lzma_index_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, lzma_index ** i, uint64_t memlimit)
{
	lzma_next_coder_init(&lzma_index_decoder_init, next, allocator);
	if(i == NULL)
		return LZMA_PROG_ERROR;
	lzma_index_decoder_coder * coder = (lzma_index_decoder_coder *)next->coder;
	if(coder == NULL) {
		coder = (lzma_index_decoder_coder *)lzma_alloc(sizeof(lzma_index_decoder_coder), allocator);
		if(coder == NULL)
			return LZMA_MEM_ERROR;
		next->coder = coder;
		next->code = &index_decode;
		next->end = &index_decoder_end;
		next->memconfig = &index_decoder_memconfig;
		coder->index = NULL;
	}
	else {
		lzma_index_end(coder->index, allocator);
	}
	return index_decoder_reset(coder, allocator, i, memlimit);
}

lzma_ret lzma_index_decoder(lzma_stream *strm, lzma_index **i, uint64_t memlimit)
{
	lzma_next_strm_init2(lzma_index_decoder_init, strm, i, memlimit);
	strm->internal->supported_actions[LZMA_RUN] = true;
	strm->internal->supported_actions[LZMA_FINISH] = true;
	return LZMA_OK;
}

lzma_ret lzma_index_buffer_decode(lzma_index **i, uint64_t *memlimit, const lzma_allocator *allocator, const uint8_t *in, size_t *in_pos, size_t in_size)
{
	// Sanity checks
	if(!i || !memlimit || !in || !in_pos || *in_pos > in_size)
		return LZMA_PROG_ERROR;
	// Initialize the decoder.
	lzma_index_decoder_coder coder;
	return_if_error(index_decoder_reset(&coder, allocator, i, *memlimit));
	// Store the input start position so that we can restore it in case of an error.
	const size_t in_start = *in_pos;
	// Do the actual decoding.
	lzma_ret ret = index_decode(&coder, allocator, in, in_pos, in_size, NULL, NULL, 0, LZMA_RUN);
	if(ret == LZMA_STREAM_END) {
		ret = LZMA_OK;
	}
	else {
		// Something went wrong, free the Index structure and restore the input position.
		lzma_index_end(coder.index, allocator);
		*in_pos = in_start;
		if(ret == LZMA_OK) {
			// The input is truncated or otherwise corrupt.
			// Use LZMA_DATA_ERROR instead of LZMA_BUF_ERROR
			// like lzma_vli_decode() does in single-call mode.
			ret = LZMA_DATA_ERROR;
		}
		else if(ret == LZMA_MEMLIMIT_ERROR) {
			// Tell the caller how much memory would have been needed.
			*memlimit = lzma_index_memusage(1, coder.count);
		}
	}
	return ret;
}
