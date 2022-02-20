/*
 * Copyright © 2018 Adobe Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * Adobe Author(s): Michiharu Ariza
 */
#ifndef HB_OT_CFF_COMMON_HH
#define HB_OT_CFF_COMMON_HH

#include "hb-open-type.hh"
#include "hb-bimap.hh"
#include "hb-ot-layout-common.hh"
#include "hb-cff-interp-dict-common.hh"
#include "hb-subset-plan.hh"

namespace CFF {
	using namespace OT;

#define CFF_UNDEF_CODE  0xFFFFFFFF

	using objidx_t = hb_serialize_context_t::objidx_t;
	using whence_t = hb_serialize_context_t::whence_t;

/* utility macro */
	template<typename Type>
	static inline const Type& StructAtOffsetOrNull(const void * P, uint offset)
	{
		return offset ? StructAtOffset<Type> (P, offset) : Null(Type);
	}

	inline uint calcOffSize(uint dataSize)
	{
		uint size = 1;
		uint offset = dataSize + 1;
		while(offset & ~0xFF) {
			size++;
			offset >>= 8;
		}
		/* format does not support size > 4; caller should handle it as an error */
		return size;
	}

	struct code_pair_t {
		hb_codepoint_t code;
		hb_codepoint_t glyph;
	};

	typedef hb_vector_t<uchar> str_buff_t;
	struct str_buff_vec_t : hb_vector_t<str_buff_t> {
		void fini() {
			SUPER::fini_deep();
		}

		uint total_size() const
		{
			uint size = 0;
			for(uint i = 0; i < length; i++)
				size += (*this)[i].length;
			return size;
		}

private:
		typedef hb_vector_t<str_buff_t> SUPER;
	};

/* CFF INDEX */
	template <typename COUNT>
	struct CFFIndex {
		static uint calculate_offset_array_size(uint offSize, uint count)
		{
			return offSize * (count + 1);
		}

		uint offset_array_size() const
		{
			return calculate_offset_array_size(offSize, count);
		}

		CFFIndex * copy(hb_serialize_context_t * c) const
		{
			TRACE_SERIALIZE(this);
			uint size = get_size();
			CFFIndex * out = c->allocate_size<CFFIndex> (size);
			if(LIKELY(out))
				memcpy(out, this, size);
			return_trace(out);
		}
		bool serialize(hb_serialize_context_t * c, const CFFIndex &src)
		{
			TRACE_SERIALIZE(this);
			uint size = src.get_size();
			CFFIndex * dest = c->allocate_size<CFFIndex> (size);
			if(UNLIKELY(!dest)) return_trace(false);
			memcpy(dest, &src, size);
			return_trace(true);
		}
		bool serialize(hb_serialize_context_t * c, uint offSize_, const byte_str_array_t &byteArray)
		{
			TRACE_SERIALIZE(this);
			if(byteArray.length == 0) {
				COUNT * dest = c->allocate_min<COUNT> ();
				if(UNLIKELY(!dest)) return_trace(false);
				*dest = 0;
			}
			else {
				/* serialize CFFIndex header */
				if(UNLIKELY(!c->extend_min(*this))) return_trace(false);
				this->count = byteArray.length;
				this->offSize = offSize_;
				if(UNLIKELY(!c->allocate_size<HBUINT8> (offSize_ * (byteArray.length + 1))))
					return_trace(false);

				/* serialize indices */
				uint offset = 1;
				uint i = 0;
				for(; i < byteArray.length; i++) {
					set_offset_at(i, offset);
					offset += byteArray[i].get_size();
				}
				set_offset_at(i, offset);

				/* serialize data */
				for(uint i = 0; i < byteArray.length; i++) {
					const byte_str_t &bs = byteArray[i];
					uchar * dest = c->allocate_size<uchar> (bs.length);
					if(UNLIKELY(!dest)) return_trace(false);
					memcpy(dest, &bs[0], bs.length);
				}
			}
			return_trace(true);
		}
		bool serialize(hb_serialize_context_t * c, uint offSize_, const str_buff_vec_t &buffArray)
		{
			byte_str_array_t byteArray;
			byteArray.init();
			byteArray.resize(buffArray.length);
			for(uint i = 0; i < byteArray.length; i++)
				byteArray[i] = byte_str_t(buffArray[i].arrayZ, buffArray[i].length);
			bool result = this->serialize(c, offSize_, byteArray);
			byteArray.fini();
			return result;
		}
		template <typename Iterator, hb_requires(hb_is_iterator(Iterator))> bool serialize(hb_serialize_context_t * c, Iterator it)
		{
			TRACE_SERIALIZE(this);
			if(it.len() == 0) {
				COUNT * dest = c->allocate_min<COUNT> ();
				if(UNLIKELY(!dest)) return_trace(false);
				*dest = 0;
			}
			else {
				serialize_header(c, +it | hb_map([] (const byte_str_t &_) { return _.length; }));
				for(const byte_str_t &_ : +it)
					_.copy(c);
			}
			return_trace(true);
		}
		bool serialize(hb_serialize_context_t * c, const byte_str_array_t &byteArray)
		{
			return serialize(c, +hb_iter(byteArray));
		}
		bool serialize(hb_serialize_context_t * c, const str_buff_vec_t &buffArray)
		{
			auto it =
			    +hb_iter(buffArray)
			    | hb_map([] (const str_buff_t &_) { return byte_str_t(_.arrayZ, _.length); })
			;
			return serialize(c, it);
		}
		template <typename Iterator, hb_requires(hb_is_iterator(Iterator))> bool serialize_header(hb_serialize_context_t * c, Iterator it)
		{
			TRACE_SERIALIZE(this);
			unsigned total = +it | hb_reduce(hb_add, 0);
			unsigned off_size = calcOffSize(total);
			/* serialize CFFIndex header */
			if(UNLIKELY(!c->extend_min(*this))) return_trace(false);
			this->count = it.len();
			this->offSize = off_size;
			if(UNLIKELY(!c->allocate_size<HBUINT8> (off_size * (it.len() + 1))))
				return_trace(false);
			/* serialize indices */
			uint offset = 1;
			uint i = 0;
			for(unsigned _ : +it) {
				CFFIndex<COUNT>::set_offset_at(i++, offset);
				offset += _;
			}
			CFFIndex<COUNT>::set_offset_at(i, offset);
			return_trace(true);
		}

		void set_offset_at(uint index, uint offset)
		{
			HBUINT8 * p = offsets + offSize * index + offSize;
			uint size = offSize;
			for(; size; size--) {
				--p;
				* p = offset & 0xFF;
				offset >>= 8;
			}
		}
		uint offset_at(uint index) const
		{
			assert(index <= count);
			const HBUINT8 * p = offsets + offSize * index;
			uint size = offSize;
			uint offset = 0;
			for(; size; size--)
				offset = (offset << 8) + *p++;
			return offset;
		}
		uint length_at(uint index) const
		{
			if(UNLIKELY((offset_at(index + 1) < offset_at(index)) || (offset_at(index + 1) > offset_at(count))))
				return 0;
			return offset_at(index + 1) - offset_at(index);
		}
		const uchar * data_base() const
		{
			return (const uchar *)this + min_size + offset_array_size();
		}
		uint data_size() const { return HBINT8::static_size; }
		byte_str_t operator [] (uint index)const
		{
			if(UNLIKELY(index >= count)) return Null(byte_str_t);
			return byte_str_t(data_base() + offset_at(index) - 1, length_at(index));
		}
		uint get_size() const
		{
			if(this == &Null(CFFIndex)) return 0;
			if(count > 0)
				return min_size + offset_array_size() + (offset_at(count) - 1);
			return count.static_size; /* empty CFFIndex contains count only */
		}
		bool sanitize(hb_sanitize_context_t * c) const
		{
			TRACE_SANITIZE(this);
			return_trace(LIKELY((c->check_struct(this) && count == 0) || /* empty INDEX */
			    (c->check_struct(this) && offSize >= 1 && offSize <= 4 &&
			    c->check_array(offsets, offSize, count + 1) &&
			    c->check_array((const HBUINT8*)data_base(), 1, max_offset() - 1))));
		}

protected:
		uint max_offset() const
		{
			uint max = 0;
			for(uint i = 0; i < count + 1u; i++) {
				uint off = offset_at(i);
				if(off > max) max = off;
			}
			return max;
		}

public:
		COUNT count; /* Number of object data. Note there are (count+1) offsets */
		HBUINT8 offSize; /* The byte size of each offset in the offsets array. */
		HBUINT8 offsets[HB_VAR_ARRAY];
		/* The array of (count + 1) offsets into objects array (1-base). */
		/* HBUINT8 data[HB_VAR_ARRAY];	Object data */
public:
		DEFINE_SIZE_ARRAY(COUNT::static_size + HBUINT8::static_size, offsets);
	};

	template <typename COUNT, typename TYPE>
	struct CFFIndexOf : CFFIndex<COUNT> {
		const byte_str_t operator [] (uint index)const
		{
			if(LIKELY(index < CFFIndex<COUNT>::count))
				return byte_str_t(CFFIndex<COUNT>::data_base() + CFFIndex<COUNT>::offset_at(index) - 1,
					   CFFIndex<COUNT>::length_at(index));
			return Null(byte_str_t);
		}

		template <typename DATA, typename PARAM1, typename PARAM2>
		bool serialize(hb_serialize_context_t * c,
		    uint offSize_,
		    const DATA * dataArray,
		    uint dataArrayLen,
		    const hb_vector_t<uint> &dataSizeArray,
		    const PARAM1 &param1,
		    const PARAM2 &param2)
		{
			TRACE_SERIALIZE(this);
			/* serialize CFFIndex header */
			if(UNLIKELY(!c->extend_min(*this))) return_trace(false);
			this->count = dataArrayLen;
			this->offSize = offSize_;
			if(UNLIKELY(!c->allocate_size<HBUINT8> (offSize_ * (dataArrayLen + 1))))
				return_trace(false);

			/* serialize indices */
			uint offset = 1;
			uint i = 0;
			for(; i < dataArrayLen; i++) {
				CFFIndex<COUNT>::set_offset_at(i, offset);
				offset += dataSizeArray[i];
			}
			CFFIndex<COUNT>::set_offset_at(i, offset);

			/* serialize data */
			for(uint i = 0; i < dataArrayLen; i++) {
				TYPE * dest = c->start_embed<TYPE> ();
				if(UNLIKELY(!dest || !dest->serialize(c, dataArray[i], param1, param2)))
					return_trace(false);
			}
			return_trace(true);
		}
	};

/* Top Dict, Font Dict, Private Dict */
	struct Dict : UnsizedByteStr {
		template <typename DICTVAL, typename OP_SERIALIZER, typename ... Ts>
		bool serialize(hb_serialize_context_t * c, const DICTVAL &dictval, OP_SERIALIZER& opszr, Ts&&... ds)
		{
			TRACE_SERIALIZE(this);
			for(uint i = 0; i < dictval.get_count(); i++)
				if(UNLIKELY(!opszr.serialize(c, dictval[i], hb_forward<Ts> (ds) ...)))
					return_trace(false);
			return_trace(true);
		}
		template <typename T, typename V> static bool serialize_int_op(hb_serialize_context_t * c, op_code_t op, V value, op_code_t intOp)
		{
			// XXX: not sure why but LLVM fails to compile the following 'unlikely' macro invocation
			if(/*unlikely*/ (!serialize_int<T, V> (c, intOp, value)))
				return false;
			TRACE_SERIALIZE(this);
			/* serialize the opcode */
			HBUINT8 * p = c->allocate_size<HBUINT8> (OpCode_Size(op));
			if(UNLIKELY(!p)) return_trace(false);
			if(Is_OpCode_ESC(op)) {
				*p = OpCode_escape;
				op = Unmake_OpCode_ESC(op);
				p++;
			}
			*p = op;
			return_trace(true);
		}
		template <typename V> static bool serialize_int4_op(hb_serialize_context_t * c, op_code_t op, V value)
		{
			return serialize_int_op<HBINT32> (c, op, value, OpCode_longintdict);
		}
		template <typename V> static bool serialize_int2_op(hb_serialize_context_t * c, op_code_t op, V value)
		{
			return serialize_int_op<HBINT16> (c, op, value, OpCode_shortint);
		}
		template <typename T, int int_op>
		static bool serialize_link_op(hb_serialize_context_t * c, op_code_t op, objidx_t link, whence_t whence)
		{
			T &ofs = *(T*)(c->head + OpCode_Size(int_op));
			if(UNLIKELY(!serialize_int_op<T> (c, op, 0, int_op))) return false;
			c->add_link(ofs, link, whence);
			return true;
		}
		static bool serialize_link4_op(hb_serialize_context_t * c, op_code_t op, objidx_t link, whence_t whence = whence_t::Head)
		{
			return serialize_link_op<HBINT32, OpCode_longintdict> (c, op, link, whence);
		}
		static bool serialize_link2_op(hb_serialize_context_t * c, op_code_t op, objidx_t link, whence_t whence = whence_t::Head)
		{
			return serialize_link_op<HBINT16, OpCode_shortint> (c, op, link, whence);
		}
	};

	struct TopDict : Dict {};
	struct FontDict : Dict {};
	struct PrivateDict : Dict {};

	struct table_info_t {
		void init() 
		{
			offset = size = 0; link = 0;
		}
		uint offset;
		uint size;
		objidx_t link;
	};

	template <typename COUNT> struct FDArray : CFFIndexOf<COUNT, FontDict> {
		template <typename DICTVAL, typename INFO, typename Iterator, typename OP_SERIALIZER>
		bool serialize(hb_serialize_context_t * c, Iterator it, OP_SERIALIZER& opszr)
		{
			TRACE_SERIALIZE(this);
			/* serialize INDEX data */
			hb_vector_t<unsigned> sizes;
			c->push();
			+it
			| hb_map([&] (const hb_pair_t<const DICTVAL&, const INFO &> &_)
			{
				FontDict * dict = c->start_embed<FontDict> ();
				dict->serialize(c, _.first, opszr, _.second);
				return c->head - (const char*)dict;
			})
			| hb_sink(sizes)
			;
			c->pop_pack(false);

			/* serialize INDEX header */
			return_trace(CFFIndex<COUNT>::serialize_header(c, hb_iter(sizes)));
		}
	};

/* FDSelect */
	struct FDSelect0 {
		bool sanitize(hb_sanitize_context_t * c, uint fdcount) const
		{
			TRACE_SANITIZE(this);
			if(UNLIKELY(!(c->check_struct(this))))
				return_trace(false);
			for(uint i = 0; i < c->get_num_glyphs(); i++)
				if(UNLIKELY(!fds[i].sanitize(c)))
					return_trace(false);

			return_trace(true);
		}

		hb_codepoint_t get_fd(hb_codepoint_t glyph) const
		{
			return (hb_codepoint_t)fds[glyph];
		}

		uint get_size(uint num_glyphs) const
		{
			return HBUINT8::static_size * num_glyphs;
		}

		HBUINT8 fds[HB_VAR_ARRAY];

		DEFINE_SIZE_MIN(0);
	};

	template <typename GID_TYPE, typename FD_TYPE>
	struct FDSelect3_4_Range {
		bool sanitize(hb_sanitize_context_t * c, const void * /*nullptr*/, uint fdcount) const
		{
			TRACE_SANITIZE(this);
			return_trace(first < c->get_num_glyphs() && (fd < fdcount));
		}

		GID_TYPE first;
		FD_TYPE fd;
public:
		DEFINE_SIZE_STATIC(GID_TYPE::static_size + FD_TYPE::static_size);
	};

	template <typename GID_TYPE, typename FD_TYPE>
	struct FDSelect3_4 {
		uint get_size() const
		{
			return GID_TYPE::static_size * 2 + ranges.get_size();
		}

		bool sanitize(hb_sanitize_context_t * c, uint fdcount) const
		{
			TRACE_SANITIZE(this);
			if(UNLIKELY(!c->check_struct(this) || !ranges.sanitize(c, nullptr, fdcount) ||
			    (nRanges() == 0) || ranges[0].first != 0))
				return_trace(false);

			for(uint i = 1; i < nRanges(); i++)
				if(UNLIKELY(ranges[i - 1].first >= ranges[i].first))
					return_trace(false);

			if(UNLIKELY(!sentinel().sanitize(c) || (sentinel() != c->get_num_glyphs())))
				return_trace(false);

			return_trace(true);
		}

		hb_codepoint_t get_fd(hb_codepoint_t glyph) const
		{
			uint i;
			for(i = 1; i < nRanges(); i++)
				if(glyph < ranges[i].first)
					break;

			return (hb_codepoint_t)ranges[i - 1].fd;
		}

		GID_TYPE        &nRanges() {
			return ranges.len;
		}

		GID_TYPE         nRanges() const {
			return ranges.len;
		}

		GID_TYPE       &sentinel() {
			return StructAfter<GID_TYPE> (ranges[nRanges() - 1]);
		}

		const GID_TYPE &sentinel() const {
			return StructAfter<GID_TYPE> (ranges[nRanges() - 1]);
		}

		ArrayOf<FDSelect3_4_Range<GID_TYPE, FD_TYPE>, GID_TYPE> ranges;
		/* GID_TYPE sentinel */

		DEFINE_SIZE_ARRAY(GID_TYPE::static_size, ranges);
	};

	typedef FDSelect3_4<HBUINT16, HBUINT8> FDSelect3;
	typedef FDSelect3_4_Range<HBUINT16, HBUINT8> FDSelect3_Range;

	struct FDSelect {
		bool serialize(hb_serialize_context_t * c, const FDSelect &src, uint num_glyphs)
		{
			TRACE_SERIALIZE(this);
			uint size = src.get_size(num_glyphs);
			FDSelect * dest = c->allocate_size<FDSelect> (size);
			if(UNLIKELY(!dest)) return_trace(false);
			memcpy(dest, &src, size);
			return_trace(true);
		}
		uint get_size(uint num_glyphs) const
		{
			switch(format) {
				case 0: return format.static_size + u.format0.get_size(num_glyphs);
				case 3: return format.static_size + u.format3.get_size();
				default: return 0;
			}
		}

		hb_codepoint_t get_fd(hb_codepoint_t glyph) const
		{
			if(this == &Null(FDSelect)) return 0;
			switch(format) {
				case 0: return u.format0.get_fd(glyph);
				case 3: return u.format3.get_fd(glyph);
				default: return 0;
			}
		}
		bool sanitize(hb_sanitize_context_t * c, uint fdcount) const
		{
			TRACE_SANITIZE(this);
			if(UNLIKELY(!c->check_struct(this)))
				return_trace(false);
			switch(format) {
				case 0: return_trace(u.format0.sanitize(c, fdcount));
				case 3: return_trace(u.format3.sanitize(c, fdcount));
				default: return_trace(false);
			}
		}

		HBUINT8 format;
		union {
			FDSelect0 format0;
			FDSelect3 format3;
		} u;

public:
		DEFINE_SIZE_MIN(1);
	};

	template <typename COUNT>
	struct Subrs : CFFIndex<COUNT> {
		typedef COUNT count_type;
		typedef CFFIndex<COUNT> SUPER;
	};
} /* namespace CFF */

#endif /* HB_OT_CFF_COMMON_HH */
