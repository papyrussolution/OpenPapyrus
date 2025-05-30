/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2016 Adrian Johnson
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is Adrian Johnson.
 *
 * Contributor(s): Adrian Johnson <ajohnson@redneon.com>
 */
/* PDF Document Interchange features:
 *  - metadata
 *  - document outline
 *  - tagged pdf
 *  - hyperlinks
 *  - page labels
 */
#include "cairoint.h"
#pragma hdrstop
#define _DEFAULT_SOURCE /* for localtime_r(), gmtime_r(), snprintf(), sstrdup() */
#include "cairo-pdf.h"
#include "cairo-pdf-surface-private.h"
#include "cairo-array-private.h"
#include "cairo-error-private.h"
#include "cairo-output-stream-private.h"
//#include <time.h>

#ifndef HAVE_LOCALTIME_R
#define localtime_r(T, BUF) (*(BUF) = *localtime(T))
#endif
#ifndef HAVE_GMTIME_R
#define gmtime_r(T, BUF) (*(BUF) = *gmtime(T))
#endif

static void write_rect_to_pdf_quad_points(cairo_output_stream_t   * stream, const cairo_rectangle_t * rect, double surface_height)
{
	_cairo_output_stream_printf(stream, "%f %f %f %f %f %f %f %f", rect->x,
	    surface_height - rect->y, rect->x + rect->width, surface_height - rect->y, rect->x + rect->width, surface_height - (rect->y + rect->height),
	    rect->x, surface_height - (rect->y + rect->height));
}

static void write_rect_int_to_pdf_bbox(cairo_output_stream_t * stream, const cairo_rectangle_int_t * rect, double surface_height)
{
	_cairo_output_stream_printf(stream, "%d %f %d %f", rect->x, surface_height - (rect->y + rect->height), rect->x + rect->width, surface_height - rect->y);
}

static cairo_int_status_t add_tree_node(cairo_pdf_surface_t * surface, cairo_pdf_struct_tree_node_t  * parent, const char * name, cairo_pdf_struct_tree_node_t ** new_node)
{
	cairo_pdf_struct_tree_node_t * node = (cairo_pdf_struct_tree_node_t *)SAlloc::M_zon0(sizeof(cairo_pdf_struct_tree_node_t));
	if(UNLIKELY(node == NULL))
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	node->name = sstrdup(name);
	node->res = _cairo_pdf_surface_new_object(surface);
	if(node->res.id == 0)
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	node->parent = parent;
	cairo_list_init(&node->children);
	_cairo_array_init(&node->mcid, sizeof(struct page_mcid));
	node->annot_res.id = 0;
	node->extents.valid = FALSE;
	cairo_list_init(&node->extents.link);
	cairo_list_add_tail(&node->link, &parent->children);
	*new_node = node;
	return CAIRO_STATUS_SUCCESS;
}

static boolint is_leaf_node(cairo_pdf_struct_tree_node_t * node)
{
	return node->parent && cairo_list_is_empty(&node->children);
}

static void free_node(cairo_pdf_struct_tree_node_t * node)
{
	cairo_pdf_struct_tree_node_t * child, * next;
	if(!node)
		return;
	cairo_list_foreach_entry_safe(child, next, cairo_pdf_struct_tree_node_t, &node->children, link)
	{
		cairo_list_del(&child->link);
		free_node(child);
	}
	SAlloc::F(node->name);
	_cairo_array_fini(&node->mcid);
	SAlloc::F(node);
}

static cairo_status_t add_mcid_to_node(cairo_pdf_surface_t * surface, cairo_pdf_struct_tree_node_t * node, int page, int * mcid)
{
	struct page_mcid mcid_elem;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_int_status_t status = _cairo_array_append(&ic->mcid_to_tree, &node);
	if(UNLIKELY(status))
		return status;
	mcid_elem.page = page;
	mcid_elem.mcid = _cairo_array_num_elements(&ic->mcid_to_tree) - 1;
	*mcid = mcid_elem.mcid;
	return _cairo_array_append(&node->mcid, &mcid_elem);
}

static cairo_int_status_t add_annotation(cairo_pdf_surface_t * surface, cairo_pdf_struct_tree_node_t * node, const char * name, const char * attributes)
{
	cairo_int_status_t status = CAIRO_STATUS_SUCCESS;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_pdf_annotation_t * annot = (cairo_pdf_annotation_t *)SAlloc::M(sizeof(cairo_pdf_annotation_t));
	if(UNLIKELY(annot == NULL))
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	status = _cairo_tag_parse_link_attributes(attributes, &annot->link_attrs);
	if(UNLIKELY(status)) {
		SAlloc::F(annot);
		return status;
	}
	annot->node = node;
	status = _cairo_array_append(&ic->annots, &annot);
	return status;
}

static void free_annotation(cairo_pdf_annotation_t * annot)
{
	if(annot) {
		_cairo_array_fini(&annot->link_attrs.rects);
		SAlloc::F(annot->link_attrs.dest);
		SAlloc::F(annot->link_attrs.uri);
		SAlloc::F(annot->link_attrs.file);
		SAlloc::F(annot);
	}
}

static void cairo_pdf_interchange_clear_annotations(cairo_pdf_surface_t * surface)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;
	int num_elems, i;

	num_elems = _cairo_array_num_elements(&ic->annots);
	for(i = 0; i < num_elems; i++) {
		cairo_pdf_annotation_t * annot;
		_cairo_array_copy_element(&ic->annots, i, &annot);
		free_annotation(annot);
	}
	_cairo_array_truncate(&ic->annots, 0);
}

static cairo_int_status_t cairo_pdf_interchange_write_node_object(cairo_pdf_surface_t * surface, cairo_pdf_struct_tree_node_t   * node)
{
	struct page_mcid * mcid_elem;
	int i, num_mcid, first_page;
	cairo_pdf_resource_t * page_res;
	cairo_pdf_struct_tree_node_t * child;
	_cairo_pdf_surface_update_object(surface, node->res);
	_cairo_output_stream_printf(surface->output,
	    "%d 0 obj\n"
	    "<< /Type /StructElem\n"
	    "   /S /%s\n"
	    "   /P %d 0 R\n",
	    node->res.id,
	    node->name,
	    node->parent->res.id);

	if(!cairo_list_is_empty(&node->children)) {
		if(cairo_list_is_singular(&node->children) && node->annot_res.id == 0) {
			child = cairo_list_first_entry(&node->children, cairo_pdf_struct_tree_node_t, link);
			_cairo_output_stream_printf(surface->output, "   /K %d 0 R\n", child->res.id);
		}
		else {
			_cairo_output_stream_printf(surface->output, "   /K [ ");
			if(node->annot_res.id != 0) {
				_cairo_output_stream_printf(surface->output,
				    "<< /Type /OBJR /Obj %d 0 R >> ",
				    node->annot_res.id);
			}
			cairo_list_foreach_entry(child, cairo_pdf_struct_tree_node_t,
			    &node->children, link)
			{
				_cairo_output_stream_printf(surface->output, "%d 0 R ", child->res.id);
			}
			_cairo_output_stream_printf(surface->output, "]\n");
		}
	}
	else {
		num_mcid = _cairo_array_num_elements(&node->mcid);
		if(num_mcid > 0) {
			mcid_elem = (struct page_mcid *)_cairo_array_index(&node->mcid, 0);
			first_page = mcid_elem->page;
			page_res = (cairo_pdf_resource_t *)_cairo_array_index(&surface->pages, first_page - 1);
			_cairo_output_stream_printf(surface->output, "   /Pg %d 0 R\n", page_res->id);

			if(num_mcid == 1 && node->annot_res.id == 0) {
				_cairo_output_stream_printf(surface->output, "   /K %d\n", mcid_elem->mcid);
			}
			else {
				_cairo_output_stream_printf(surface->output, "   /K [ ");
				if(node->annot_res.id != 0) {
					_cairo_output_stream_printf(surface->output, "<< /Type /OBJR /Obj %d 0 R >> ", node->annot_res.id);
				}
				for(i = 0; i < num_mcid; i++) {
					mcid_elem = (struct page_mcid *)_cairo_array_index(&node->mcid, i);
					page_res = (cairo_pdf_resource_t *)_cairo_array_index(&surface->pages, mcid_elem->page - 1);
					if(mcid_elem->page == first_page) {
						_cairo_output_stream_printf(surface->output, "%d ", mcid_elem->mcid);
					}
					else {
						_cairo_output_stream_printf(surface->output, "\n       << /Type /MCR /Pg %d 0 R /MCID %d >> ",
						    page_res->id, mcid_elem->mcid);
					}
				}
				_cairo_output_stream_printf(surface->output, "]\n");
			}
		}
	}
	_cairo_output_stream_printf(surface->output, ">>\nendobj\n");
	return _cairo_output_stream_get_status(surface->output);
}

static void init_named_dest_key(cairo_pdf_named_dest_t * dest)
{
	dest->base.hash = _cairo_hash_bytes(SlConst::DjbHashInit32, dest->attrs.name, strlen(dest->attrs.name));
}

static boolint _named_dest_equal(const void * key_a, const void * key_b)
{
	const cairo_pdf_named_dest_t * a = (const cairo_pdf_named_dest_t *)key_a;
	const cairo_pdf_named_dest_t * b = (const cairo_pdf_named_dest_t *)key_b;
	return sstreq(a->attrs.name, b->attrs.name);
}

static void _named_dest_pluck(void * entry, void * closure)
{
	cairo_pdf_named_dest_t * dest = (cairo_pdf_named_dest_t *)entry;
	cairo_hash_table_t * table = (cairo_hash_table_t *)closure;
	_cairo_hash_table_remove(table, &dest->base);
	SAlloc::F(dest->attrs.name);
	SAlloc::F(dest);
}

static cairo_int_status_t cairo_pdf_interchange_write_explicit_dest(cairo_pdf_surface_t * surface, int page, boolint has_pos, double x, double y)
{
	cairo_pdf_resource_t res;
	double height;
	if(page < 1 || page > (int)_cairo_array_num_elements(&surface->pages)) {
		//return CAIRO_INT_STATUS_TAG_ERROR;
		return CAIRO_STATUS_TAG_ERROR;
	}
	_cairo_array_copy_element(&surface->page_heights, page - 1, &height);
	_cairo_array_copy_element(&surface->pages, page - 1, &res);
	if(has_pos) {
		_cairo_output_stream_printf(surface->output, "   /Dest [%d 0 R /XYZ %f %f 0]\n", res.id, x, height - y);
	}
	else {
		_cairo_output_stream_printf(surface->output, "   /Dest [%d 0 R /XYZ null null 0]\n", res.id);
	}
	return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t cairo_pdf_interchange_write_dest(cairo_pdf_surface_t * surface, cairo_link_attrs_t  * link_attrs)
{
	cairo_int_status_t status;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	char * dest = NULL;
	if(link_attrs->dest) {
		cairo_pdf_named_dest_t key;
		cairo_pdf_named_dest_t * named_dest;
		/* check if this is a link to an internal named dest */
		key.attrs.name = link_attrs->dest;
		init_named_dest_key(&key);
		named_dest = (cairo_pdf_named_dest_t *)_cairo_hash_table_lookup(ic->named_dests, &key.base);
		if(named_dest && named_dest->attrs.internal) {
			/* if dests exists and has internal attribute, use a direct
			 * reference instead of the name */
			double x = 0;
			double y = 0;
			if(named_dest->extents.valid) {
				x = named_dest->extents.extents.x;
				y = named_dest->extents.extents.y;
			}
			if(named_dest->attrs.x_valid)
				x = named_dest->attrs.x;
			if(named_dest->attrs.y_valid)
				y = named_dest->attrs.y;
			status = cairo_pdf_interchange_write_explicit_dest(surface, named_dest->page, TRUE, x, y);
			return status;
		}
	}

	if(link_attrs->dest) {
		status = _cairo_utf8_to_pdf_string(link_attrs->dest, &dest);
		if(UNLIKELY(status))
			return status;
		_cairo_output_stream_printf(surface->output, "   /Dest %s\n", dest);
		SAlloc::F(dest);
	}
	else {
		status = cairo_pdf_interchange_write_explicit_dest(surface, link_attrs->page, link_attrs->has_pos, link_attrs->pos.x, link_attrs->pos.y);
	}
	return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t cairo_pdf_interchange_write_link_action(cairo_pdf_surface_t * surface, cairo_link_attrs_t * link_attrs)
{
	cairo_int_status_t status;
	char * dest = NULL;
	if(link_attrs->link_type == TAG_LINK_DEST) {
		status = cairo_pdf_interchange_write_dest(surface, link_attrs);
		if(UNLIKELY(status))
			return status;
	}
	else if(link_attrs->link_type == TAG_LINK_URI) {
		_cairo_output_stream_printf(surface->output,
		    "   /A <<\n"
		    "      /Type /Action\n"
		    "      /S /URI\n"
		    "      /URI (%s)\n"
		    "   >>\n",
		    link_attrs->uri);
	}
	else if(link_attrs->link_type == TAG_LINK_FILE) {
		_cairo_output_stream_printf(surface->output,
		    "   /A <<\n"
		    "      /Type /Action\n"
		    "      /S /GoToR\n"
		    "      /F (%s)\n",
		    link_attrs->file);
		if(link_attrs->dest) {
			status = _cairo_utf8_to_pdf_string(link_attrs->dest, &dest);
			if(UNLIKELY(status))
				return status;

			_cairo_output_stream_printf(surface->output,
			    "      /D %s\n",
			    dest);
			SAlloc::F(dest);
		}
		else {
			if(link_attrs->has_pos) {
				_cairo_output_stream_printf(surface->output,
				    "      /D [%d %f %f 0]\n",
				    link_attrs->page,
				    link_attrs->pos.x,
				    link_attrs->pos.y);
			}
			else {
				_cairo_output_stream_printf(surface->output,
				    "      /D [%d null null 0]\n",
				    link_attrs->page);
			}
		}
		_cairo_output_stream_printf(surface->output,
		    "   >>\n");
	}

	return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t cairo_pdf_interchange_write_annot(cairo_pdf_surface_t    * surface,
    cairo_pdf_annotation_t * annot)
{
	cairo_int_status_t status = CAIRO_STATUS_SUCCESS;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_pdf_struct_tree_node_t * node = annot->node;
	int sp;
	int i, num_rects;
	double height;

	num_rects = _cairo_array_num_elements(&annot->link_attrs.rects);
	if(sstreq(node->name, CAIRO_TAG_LINK) && annot->link_attrs.link_type != TAG_LINK_EMPTY && (node->extents.valid || num_rects > 0)) {
		status = _cairo_array_append(&ic->parent_tree, &node->res);
		if(UNLIKELY(status))
			return status;
		sp = _cairo_array_num_elements(&ic->parent_tree) - 1;
		node->annot_res = _cairo_pdf_surface_new_object(surface);
		status = _cairo_array_append(&surface->page_annots, &node->annot_res);
		if(UNLIKELY(status))
			return status;
		_cairo_pdf_surface_update_object(surface, node->annot_res);
		_cairo_output_stream_printf(surface->output,
		    "%d 0 obj\n"
		    "<< /Type /Annot\n"
		    "   /Subtype /Link\n"
		    "   /StructParent %d\n",
		    node->annot_res.id,
		    sp);

		height = surface->height;
		if(num_rects > 0) {
			cairo_rectangle_int_t bbox_rect;

			_cairo_output_stream_printf(surface->output,
			    "   /QuadPoints [ ");
			for(i = 0; i < num_rects; i++) {
				cairo_rectangle_t rectf;
				cairo_rectangle_int_t recti;

				_cairo_array_copy_element(&annot->link_attrs.rects, i, &rectf);
				_cairo_rectangle_int_from_double(&recti, &rectf);
				if(!i)
					bbox_rect = recti;
				else
					_cairo_rectangle_union(&bbox_rect, &recti);

				write_rect_to_pdf_quad_points(surface->output, &rectf, height);
				_cairo_output_stream_printf(surface->output, " ");
			}
			_cairo_output_stream_printf(surface->output,
			    "]\n"
			    "   /Rect [ ");
			write_rect_int_to_pdf_bbox(surface->output, &bbox_rect, height);
			_cairo_output_stream_printf(surface->output, " ]\n");
		}
		else {
			_cairo_output_stream_printf(surface->output,
			    "   /Rect [ ");
			write_rect_int_to_pdf_bbox(surface->output, &node->extents.extents, height);
			_cairo_output_stream_printf(surface->output, " ]\n");
		}

		status = cairo_pdf_interchange_write_link_action(surface, &annot->link_attrs);
		if(UNLIKELY(status))
			return status;

		_cairo_output_stream_printf(surface->output,
		    "   /BS << /W 0 >>"
		    ">>\n"
		    "endobj\n");

		status = _cairo_output_stream_get_status(surface->output);
	}

	return status;
}

static cairo_int_status_t cairo_pdf_interchange_walk_struct_tree(cairo_pdf_surface_t          * surface,
    cairo_pdf_struct_tree_node_t * node,
    cairo_int_status_t (*func)(cairo_pdf_surface_t * surface,
    cairo_pdf_struct_tree_node_t * node))
{
	cairo_int_status_t status;
	cairo_pdf_struct_tree_node_t * child;

	if(node->parent) {
		status = func(surface, node);
		if(UNLIKELY(status))
			return status;
	}

	cairo_list_foreach_entry(child, cairo_pdf_struct_tree_node_t,
	    &node->children, link)
	{
		status = cairo_pdf_interchange_walk_struct_tree(surface, child, func);
		if(UNLIKELY(status))
			return status;
	}

	return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t cairo_pdf_interchange_write_struct_tree(cairo_pdf_surface_t * surface)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_pdf_struct_tree_node_t * child;

	if(cairo_list_is_empty(&ic->struct_root->children))
		return CAIRO_STATUS_SUCCESS;

	surface->struct_tree_root = _cairo_pdf_surface_new_object(surface);
	ic->struct_root->res = surface->struct_tree_root;

	cairo_pdf_interchange_walk_struct_tree(surface, ic->struct_root, cairo_pdf_interchange_write_node_object);

	child = cairo_list_first_entry(&ic->struct_root->children, cairo_pdf_struct_tree_node_t, link);
	_cairo_pdf_surface_update_object(surface, surface->struct_tree_root);
	_cairo_output_stream_printf(surface->output,
	    "%d 0 obj\n"
	    "<< /Type /StructTreeRoot\n"
	    "   /ParentTree %d 0 R\n",
	    surface->struct_tree_root.id,
	    ic->parent_tree_res.id);

	if(cairo_list_is_singular(&ic->struct_root->children)) {
		child = cairo_list_first_entry(&ic->struct_root->children, cairo_pdf_struct_tree_node_t, link);
		_cairo_output_stream_printf(surface->output, "   /K [ %d 0 R ]\n", child->res.id);
	}
	else {
		_cairo_output_stream_printf(surface->output, "   /K [ ");

		cairo_list_foreach_entry(child, cairo_pdf_struct_tree_node_t,
		    &ic->struct_root->children, link)
		{
			_cairo_output_stream_printf(surface->output, "%d 0 R ", child->res.id);
		}
		_cairo_output_stream_printf(surface->output, "]\n");
	}

	_cairo_output_stream_printf(surface->output,
	    ">>\n"
	    "endobj\n");

	return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t cairo_pdf_interchange_write_page_annots(cairo_pdf_surface_t * surface)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;
	int num_elems, i;
	cairo_int_status_t status = CAIRO_STATUS_SUCCESS;

	num_elems = _cairo_array_num_elements(&ic->annots);
	for(i = 0; i < num_elems; i++) {
		cairo_pdf_annotation_t * annot;

		_cairo_array_copy_element(&ic->annots, i, &annot);
		status = cairo_pdf_interchange_write_annot(surface, annot);
		if(UNLIKELY(status))
			return status;
	}

	return status;
}

static cairo_int_status_t cairo_pdf_interchange_write_page_parent_elems(cairo_pdf_surface_t * surface)
{
	int num_elems, i;
	cairo_pdf_struct_tree_node_t * node;
	cairo_pdf_resource_t res;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_int_status_t status = CAIRO_STATUS_SUCCESS;

	surface->page_parent_tree = -1;
	num_elems = _cairo_array_num_elements(&ic->mcid_to_tree);
	if(num_elems > 0) {
		res = _cairo_pdf_surface_new_object(surface);
		_cairo_output_stream_printf(surface->output,
		    "%d 0 obj\n"
		    "[\n",
		    res.id);
		for(i = 0; i < num_elems; i++) {
			_cairo_array_copy_element(&ic->mcid_to_tree, i, &node);
			_cairo_output_stream_printf(surface->output, "  %d 0 R\n", node->res.id);
		}
		_cairo_output_stream_printf(surface->output,
		    "]\n"
		    "endobj\n");
		status = _cairo_array_append(&ic->parent_tree, &res);
		surface->page_parent_tree = _cairo_array_num_elements(&ic->parent_tree) - 1;
	}

	return status;
}

static cairo_int_status_t cairo_pdf_interchange_write_parent_tree(cairo_pdf_surface_t * surface)
{
	int i;
	cairo_pdf_resource_t * res;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	int num_elems = _cairo_array_num_elements(&ic->parent_tree);
	if(num_elems > 0) {
		ic->parent_tree_res = _cairo_pdf_surface_new_object(surface);
		_cairo_output_stream_printf(surface->output,
		    "%d 0 obj\n"
		    "<< /Nums [\n",
		    ic->parent_tree_res.id);
		for(i = 0; i < num_elems; i++) {
			res = (cairo_pdf_resource_t *)_cairo_array_index(&ic->parent_tree, i);
			if(res->id) {
				_cairo_output_stream_printf(surface->output, "   %d %d 0 R\n", i, res->id);
			}
		}
		_cairo_output_stream_printf(surface->output,
		    "  ]\n"
		    ">>\n"
		    "endobj\n");
	}
	return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t cairo_pdf_interchange_write_outline(cairo_pdf_surface_t * surface)
{
	int num_elems, i;
	cairo_pdf_outline_entry_t * outline;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_int_status_t status;
	char * name = NULL;

	num_elems = _cairo_array_num_elements(&ic->outline);
	if(num_elems < 2)
		return CAIRO_INT_STATUS_SUCCESS;

	_cairo_array_copy_element(&ic->outline, 0, &outline);
	outline->res = _cairo_pdf_surface_new_object(surface);
	surface->outlines_dict_res = outline->res;
	_cairo_output_stream_printf(surface->output,
	    "%d 0 obj\n"
	    "<< /Type /Outlines\n"
	    "   /First %d 0 R\n"
	    "   /Last %d 0 R\n"
	    "   /Count %d\n"
	    ">>\n"
	    "endobj\n",
	    outline->res.id,
	    outline->first_child->res.id,
	    outline->last_child->res.id,
	    outline->count);

	for(i = 1; i < num_elems; i++) {
		_cairo_array_copy_element(&ic->outline, i, &outline);
		_cairo_pdf_surface_update_object(surface, outline->res);

		status = _cairo_utf8_to_pdf_string(outline->name, &name);
		if(UNLIKELY(status))
			return status;

		_cairo_output_stream_printf(surface->output,
		    "%d 0 obj\n"
		    "<< /Title %s\n"
		    "   /Parent %d 0 R\n",
		    outline->res.id,
		    name,
		    outline->parent->res.id);
		SAlloc::F(name);

		if(outline->prev) {
			_cairo_output_stream_printf(surface->output,
			    "   /Prev %d 0 R\n",
			    outline->prev->res.id);
		}

		if(outline->next) {
			_cairo_output_stream_printf(surface->output,
			    "   /Next %d 0 R\n",
			    outline->next->res.id);
		}

		if(outline->first_child) {
			_cairo_output_stream_printf(surface->output,
			    "   /First %d 0 R\n"
			    "   /Last %d 0 R\n"
			    "   /Count %d\n",
			    outline->first_child->res.id,
			    outline->last_child->res.id,
			    outline->count);
		}

		if(outline->flags) {
			int flags = 0;
			if(outline->flags & CAIRO_PDF_OUTLINE_FLAG_ITALIC)
				flags |= 1;
			if(outline->flags & CAIRO_PDF_OUTLINE_FLAG_BOLD)
				flags |= 2;
			_cairo_output_stream_printf(surface->output,
			    "   /F %d\n",
			    flags);
		}

		status = cairo_pdf_interchange_write_link_action(surface, &outline->link_attrs);
		if(UNLIKELY(status))
			return status;

		_cairo_output_stream_printf(surface->output,
		    ">>\n"
		    "endobj\n");
	}

	return status;
}

/*
 * Split a page label into a text prefix and numeric suffix. Leading '0's are
 * included in the prefix. eg
 *  "3"     => NULL,    3
 *  "cover" => "cover", 0
 *  "A-2"   => "A-",    2
 *  "A-002" => "A-00",  2
 */
static char * split_label(const char * label, int * num)
{
	int len, i;

	*num = 0;
	len = strlen(label);
	if(!len)
		return NULL;

	i = len;
	while(i > 0 && isdec(label[i-1]))
		i--;

	while(i < len && label[i] == '0')
		i++;

	if(i < len)
		sscanf(label + i, "%d", num);

	if(i > 0) {
		char * s = (char *)SAlloc::M_zon0(i + 1);
		if(!s)
			return NULL;
		memcpy(s, label, i);
		s[i] = 0;
		return s;
	}
	return NULL;
}
//
// strcmp that handles NULL arguments 
//
static boolint strcmp_null(const char * s1, const char * s2)
{
	if(s1 && s2)
		return strcmp(s1, s2) == 0;
	if(!s1 && !s2)
		return TRUE;
	return FALSE;
}

static cairo_int_status_t cairo_pdf_interchange_write_page_labels(cairo_pdf_surface_t * surface)
{
	int num_elems, i;
	char * label;
	char * prefix;
	char * prev_prefix;
	int num, prev_num;
	cairo_int_status_t status;
	boolint has_labels;
	// Check if any labels defined 
	num_elems = _cairo_array_num_elements(&surface->page_labels);
	has_labels = FALSE;
	for(i = 0; i < num_elems; i++) {
		_cairo_array_copy_element(&surface->page_labels, i, &label);
		if(label) {
			has_labels = TRUE;
			break;
		}
	}
	if(!has_labels)
		return CAIRO_STATUS_SUCCESS;
	surface->page_labels_res = _cairo_pdf_surface_new_object(surface);
	_cairo_output_stream_printf(surface->output,
	    "%d 0 obj\n"
	    "<< /Nums [\n",
	    surface->page_labels_res.id);
	prefix = NULL;
	prev_prefix = NULL;
	num = 0;
	prev_num = 0;
	for(i = 0; i < num_elems; i++) {
		_cairo_array_copy_element(&surface->page_labels, i, &label);
		if(label) {
			prefix = split_label(label, &num);
		}
		else {
			prefix = NULL;
			num = i + 1;
		}
		if(!strcmp_null(prefix, prev_prefix) || num != prev_num + 1) {
			_cairo_output_stream_printf(surface->output,  "   %d << ", i);
			if(num)
				_cairo_output_stream_printf(surface->output,  "/S /D /St %d ", num);
			if(prefix) {
				char * s;
				status = _cairo_utf8_to_pdf_string(prefix, &s);
				if(UNLIKELY(status))
					return status;
				_cairo_output_stream_printf(surface->output,  "/P %s ", s);
				SAlloc::F(s);
			}
			_cairo_output_stream_printf(surface->output,  ">>\n");
		}
		SAlloc::F(prev_prefix);
		prev_prefix = prefix;
		prefix = NULL;
		prev_num = num;
	}
	SAlloc::F(prefix);
	SAlloc::F(prev_prefix);
	_cairo_output_stream_printf(surface->output,
	    "  ]\n"
	    ">>\n"
	    "endobj\n");

	return CAIRO_STATUS_SUCCESS;
}

static void _collect_dest(void * entry, void * closure)
{
	cairo_pdf_named_dest_t * dest = (cairo_pdf_named_dest_t *)entry;
	cairo_pdf_surface_t * surface = (cairo_pdf_surface_t *)closure;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	ic->sorted_dests[ic->num_dests++] = dest;
}

static int _dest_compare(const void * a, const void * b)
{
	const cairo_pdf_named_dest_t * const * dest_a = (const cairo_pdf_named_dest_t * const *)a;
	const cairo_pdf_named_dest_t * const * dest_b = (const cairo_pdf_named_dest_t * const *)b;
	return strcmp((*dest_a)->attrs.name, (*dest_b)->attrs.name);
}

static cairo_int_status_t _cairo_pdf_interchange_write_document_dests(cairo_pdf_surface_t * surface)
{
	int i;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	if(ic->num_dests == 0) {
		ic->dests_res.id = 0;
		return CAIRO_STATUS_SUCCESS;
	}
	ic->sorted_dests = (cairo_pdf_named_dest_t **)SAlloc::C(ic->num_dests, sizeof(cairo_pdf_named_dest_t *));
	if(UNLIKELY(ic->sorted_dests == NULL))
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	ic->num_dests = 0;
	_cairo_hash_table_foreach(ic->named_dests, _collect_dest, surface);
	qsort(ic->sorted_dests, ic->num_dests, sizeof(cairo_pdf_named_dest_t *), _dest_compare);
	ic->dests_res = _cairo_pdf_surface_new_object(surface);
	_cairo_output_stream_printf(surface->output, "%d 0 obj\n<< /Names [\n", ic->dests_res.id);
	for(i = 0; i < ic->num_dests; i++) {
		cairo_pdf_named_dest_t * dest = ic->sorted_dests[i];
		cairo_pdf_resource_t page_res;
		double x = 0;
		double y = 0;
		double height;
		if(dest->attrs.internal)
			continue;
		if(dest->extents.valid) {
			x = dest->extents.extents.x;
			y = dest->extents.extents.y;
		}

		if(dest->attrs.x_valid)
			x = dest->attrs.x;

		if(dest->attrs.y_valid)
			y = dest->attrs.y;

		_cairo_array_copy_element(&surface->pages, dest->page - 1, &page_res);
		_cairo_array_copy_element(&surface->page_heights, dest->page - 1, &height);
		_cairo_output_stream_printf(surface->output,
		    "   (%s) [%d 0 R /XYZ %f %f 0]\n",
		    dest->attrs.name,
		    page_res.id,
		    x,
		    height - y);
	}
	_cairo_output_stream_printf(surface->output,
	    "  ]\n"
	    ">>\n"
	    "endobj\n");

	return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t cairo_pdf_interchange_write_names_dict(cairo_pdf_surface_t * surface)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_int_status_t status;

	status = _cairo_pdf_interchange_write_document_dests(surface);
	if(UNLIKELY(status))
		return status;

	surface->names_dict_res.id = 0;
	if(ic->dests_res.id != 0) {
		surface->names_dict_res = _cairo_pdf_surface_new_object(surface);
		_cairo_output_stream_printf(surface->output,
		    "%d 0 obj\n"
		    "<< /Dests %d 0 R >>\n"
		    "endobj\n",
		    surface->names_dict_res.id,
		    ic->dests_res.id);
	}

	return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t cairo_pdf_interchange_write_docinfo(cairo_pdf_surface_t * surface)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;

	surface->docinfo_res = _cairo_pdf_surface_new_object(surface);
	if(surface->docinfo_res.id == 0)
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);

	_cairo_output_stream_printf(surface->output,
	    "%d 0 obj\n"
	    "<< /Producer (cairo %s (https://cairographics.org))\n",
	    surface->docinfo_res.id,
	    cairo_version_string());

	if(ic->docinfo.title)
		_cairo_output_stream_printf(surface->output, "   /Title %s\n", ic->docinfo.title);

	if(ic->docinfo.author)
		_cairo_output_stream_printf(surface->output, "   /Author %s\n", ic->docinfo.author);

	if(ic->docinfo.subject)
		_cairo_output_stream_printf(surface->output, "   /Subject %s\n", ic->docinfo.subject);

	if(ic->docinfo.keywords)
		_cairo_output_stream_printf(surface->output, "   /Keywords %s\n", ic->docinfo.keywords);

	if(ic->docinfo.creator)
		_cairo_output_stream_printf(surface->output, "   /Creator %s\n", ic->docinfo.creator);

	if(ic->docinfo.create_date)
		_cairo_output_stream_printf(surface->output, "   /CreationDate %s\n", ic->docinfo.create_date);

	if(ic->docinfo.mod_date)
		_cairo_output_stream_printf(surface->output, "   /ModDate %s\n", ic->docinfo.mod_date);

	_cairo_output_stream_printf(surface->output,
	    ">>\n"
	    "endobj\n");

	return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t _cairo_pdf_interchange_begin_structure_tag(cairo_pdf_surface_t    * surface,
    cairo_tag_type_t tag_type,
    const char             * name,
    const char             * attributes)
{
	int page_num, mcid;
	cairo_int_status_t status = CAIRO_STATUS_SUCCESS;
	cairo_pdf_interchange_t * ic = &surface->interchange;

	if(surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
		status = add_tree_node(surface, ic->current_node, name, &ic->current_node);
		if(UNLIKELY(status))
			return status;

		_cairo_tag_stack_set_top_data(&ic->analysis_tag_stack, ic->current_node);

		if(tag_type & TAG_TYPE_LINK) {
			status = add_annotation(surface, ic->current_node, name, attributes);
			if(UNLIKELY(status))
				return status;

			cairo_list_add_tail(&ic->current_node->extents.link, &ic->extents_list);
		}
	}
	else if(surface->paginated_mode == CAIRO_PAGINATED_MODE_RENDER) {
		ic->current_node = (cairo_pdf_struct_tree_node_t *)_cairo_tag_stack_top_elem(&ic->render_tag_stack)->data;
		assert(ic->current_node != NULL);
		if(is_leaf_node(ic->current_node)) {
			page_num = _cairo_array_num_elements(&surface->pages);
			add_mcid_to_node(surface, ic->current_node, page_num, &mcid);
			status = _cairo_pdf_operators_tag_begin(&surface->pdf_operators, name, mcid);
		}
	}
	return status;
}

static cairo_int_status_t _cairo_pdf_interchange_begin_dest_tag(cairo_pdf_surface_t * surface, cairo_tag_type_t tag_type, const char * name, const char * attributes)
{
	cairo_pdf_named_dest_t * dest;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_int_status_t status = CAIRO_STATUS_SUCCESS;
	if(surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
		dest = (cairo_pdf_named_dest_t *)SAlloc::C(1, sizeof(cairo_pdf_named_dest_t));
		if(UNLIKELY(dest == NULL))
			return _cairo_error(CAIRO_STATUS_NO_MEMORY);
		status = _cairo_tag_parse_dest_attributes(attributes, &dest->attrs);
		if(UNLIKELY(status))
			return status;
		dest->page = _cairo_array_num_elements(&surface->pages);
		init_named_dest_key(dest);
		status = _cairo_hash_table_insert(ic->named_dests, &dest->base);
		if(UNLIKELY(status))
			return status;
		_cairo_tag_stack_set_top_data(&ic->analysis_tag_stack, dest);
		cairo_list_add_tail(&dest->extents.link, &ic->extents_list);
		ic->num_dests++;
	}
	return status;
}

cairo_int_status_t _cairo_pdf_interchange_tag_begin(cairo_pdf_surface_t * surface, const char * name, const char * attributes)
{
	cairo_int_status_t status = CAIRO_STATUS_SUCCESS;
	cairo_tag_type_t tag_type;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	void * ptr;
	if(surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
		status = _cairo_tag_stack_push(&ic->analysis_tag_stack, name, attributes);
	}
	else if(surface->paginated_mode == CAIRO_PAGINATED_MODE_RENDER) {
		status = _cairo_tag_stack_push(&ic->render_tag_stack, name, attributes);
		_cairo_array_copy_element(&ic->push_data, ic->push_data_index++, &ptr);
		_cairo_tag_stack_set_top_data(&ic->render_tag_stack, ptr);
	}

	if(UNLIKELY(status))
		return status;

	tag_type = _cairo_tag_get_type(name);
	if(tag_type & TAG_TYPE_STRUCTURE) {
		status = _cairo_pdf_interchange_begin_structure_tag(surface, tag_type, name, attributes);
		if(UNLIKELY(status))
			return status;
	}

	if(tag_type & TAG_TYPE_DEST) {
		status = _cairo_pdf_interchange_begin_dest_tag(surface, tag_type, name, attributes);
		if(UNLIKELY(status))
			return status;
	}

	if(surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
		ptr = _cairo_tag_stack_top_elem(&ic->analysis_tag_stack)->data;
		status = _cairo_array_append(&ic->push_data, &ptr);
	}

	return status;
}

static cairo_int_status_t _cairo_pdf_interchange_end_structure_tag(cairo_pdf_surface_t    * surface,
    cairo_tag_type_t tag_type,
    cairo_tag_stack_elem_t * elem)
{
	const cairo_pdf_struct_tree_node_t * node;
	struct tag_extents * tag, * next;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_int_status_t status = CAIRO_STATUS_SUCCESS;
	assert(elem->data != NULL);
	node = (const cairo_pdf_struct_tree_node_t *)elem->data;
	if(surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
		if(tag_type & TAG_TYPE_LINK) {
			cairo_list_foreach_entry_safe(tag, next, struct tag_extents,
			    &ic->extents_list, link) {
				if(tag == &node->extents) {
					cairo_list_del(&tag->link);
					break;
				}
			}
		}
	}
	else if(surface->paginated_mode == CAIRO_PAGINATED_MODE_RENDER) {
		if(is_leaf_node(ic->current_node)) {
			status = _cairo_pdf_operators_tag_end(&surface->pdf_operators);
			if(UNLIKELY(status))
				return status;
		}
	}

	ic->current_node = ic->current_node->parent;
	assert(ic->current_node != NULL);

	return status;
}

static cairo_int_status_t _cairo_pdf_interchange_end_dest_tag(cairo_pdf_surface_t    * surface,
    cairo_tag_type_t tag_type,
    cairo_tag_stack_elem_t * elem)
{
	struct tag_extents * tag, * next;
	cairo_pdf_named_dest_t * dest;
	cairo_pdf_interchange_t * ic = &surface->interchange;

	if(surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
		assert(elem->data != NULL);
		dest = (cairo_pdf_named_dest_t*)elem->data;
		cairo_list_foreach_entry_safe(tag, next, struct tag_extents,
		    &ic->extents_list, link) {
			if(tag == &dest->extents) {
				cairo_list_del(&tag->link);
				break;
			}
		}
	}

	return CAIRO_STATUS_SUCCESS;
}

cairo_int_status_t _cairo_pdf_interchange_tag_end(cairo_pdf_surface_t * surface,
    const char * name)
{
	cairo_int_status_t status = CAIRO_STATUS_SUCCESS;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_tag_type_t tag_type;
	cairo_tag_stack_elem_t * elem;

	if(surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE)
		status = _cairo_tag_stack_pop(&ic->analysis_tag_stack, name, &elem);
	else if(surface->paginated_mode == CAIRO_PAGINATED_MODE_RENDER)
		status = _cairo_tag_stack_pop(&ic->render_tag_stack, name, &elem);

	if(UNLIKELY(status))
		return status;

	tag_type = _cairo_tag_get_type(name);
	if(tag_type & TAG_TYPE_STRUCTURE) {
		status = _cairo_pdf_interchange_end_structure_tag(surface, tag_type, elem);
		if(UNLIKELY(status))
			goto cleanup;
	}

	if(tag_type & TAG_TYPE_DEST) {
		status = _cairo_pdf_interchange_end_dest_tag(surface, tag_type, elem);
		if(UNLIKELY(status))
			goto cleanup;
	}

cleanup:
	_cairo_tag_stack_free_elem(elem);

	return status;
}

cairo_int_status_t _cairo_pdf_interchange_add_operation_extents(cairo_pdf_surface_t         * surface,
    const cairo_rectangle_int_t * extents)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;
	struct tag_extents * tag;

	if(surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
		cairo_list_foreach_entry(tag, struct tag_extents, &ic->extents_list, link) {
			if(tag->valid) {
				_cairo_rectangle_union(&tag->extents, extents);
			}
			else {
				tag->extents = *extents;
				tag->valid = TRUE;
			}
		}
	}

	return CAIRO_STATUS_SUCCESS;
}

cairo_int_status_t _cairo_pdf_interchange_begin_page_content(cairo_pdf_surface_t * surface)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_int_status_t status = CAIRO_STATUS_SUCCESS;
	int page_num, mcid;

	if(surface->paginated_mode == CAIRO_PAGINATED_MODE_ANALYZE) {
		_cairo_array_truncate(&ic->mcid_to_tree, 0);
		_cairo_array_truncate(&ic->push_data, 0);
		ic->begin_page_node = ic->current_node;
	}
	else if(surface->paginated_mode == CAIRO_PAGINATED_MODE_RENDER) {
		ic->push_data_index = 0;
		ic->current_node = ic->begin_page_node;
		if(ic->end_page_node && is_leaf_node(ic->end_page_node)) {
			page_num = _cairo_array_num_elements(&surface->pages);
			add_mcid_to_node(surface, ic->end_page_node, page_num, &mcid);
			status = _cairo_pdf_operators_tag_begin(&surface->pdf_operators,
				ic->end_page_node->name,
				mcid);
		}
	}

	return status;
}

cairo_int_status_t _cairo_pdf_interchange_end_page_content(cairo_pdf_surface_t * surface)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_int_status_t status = CAIRO_STATUS_SUCCESS;

	if(surface->paginated_mode == CAIRO_PAGINATED_MODE_RENDER) {
		ic->end_page_node = ic->current_node;
		if(is_leaf_node(ic->current_node))
			status = _cairo_pdf_operators_tag_end(&surface->pdf_operators);
	}

	return status;
}

cairo_int_status_t _cairo_pdf_interchange_write_page_objects(cairo_pdf_surface_t * surface)
{
	cairo_int_status_t status;

	status = cairo_pdf_interchange_write_page_annots(surface);
	if(UNLIKELY(status))
		return status;

	cairo_pdf_interchange_clear_annotations(surface);

	return cairo_pdf_interchange_write_page_parent_elems(surface);
}

cairo_int_status_t _cairo_pdf_interchange_write_document_objects(cairo_pdf_surface_t * surface)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_int_status_t status = CAIRO_STATUS_SUCCESS;
	cairo_tag_stack_structure_type_t tag_type = _cairo_tag_stack_get_structure_type(&ic->analysis_tag_stack);
	if(tag_type == TAG_TREE_TYPE_TAGGED || tag_type == TAG_TREE_TYPE_STRUCTURE) {
		status = cairo_pdf_interchange_write_parent_tree(surface);
		if(UNLIKELY(status))
			return status;
		status = cairo_pdf_interchange_write_struct_tree(surface);
		if(UNLIKELY(status))
			return status;
		if(tag_type == TAG_TREE_TYPE_TAGGED)
			surface->tagged = TRUE;
	}
	status = cairo_pdf_interchange_write_outline(surface);
	if(UNLIKELY(status))
		return status;
	status = cairo_pdf_interchange_write_page_labels(surface);
	if(UNLIKELY(status))
		return status;
	status = cairo_pdf_interchange_write_names_dict(surface);
	if(UNLIKELY(status))
		return status;
	status = cairo_pdf_interchange_write_docinfo(surface);
	return status;
}

static void _cairo_pdf_interchange_set_create_date(cairo_pdf_surface_t * surface)
{
	time_t local, offset;
	struct tm tm_local, tm_utc;
	char buf[50];
	int buf_size;
	char * p;
	cairo_pdf_interchange_t * ic = &surface->interchange;
	time_t utc = time(NULL);
	localtime_r(&utc, &tm_local);
	strftime(buf, sizeof(buf), "(D:%Y%m%d%H%M%S", &tm_local);
	/* strftime "%z" is non standard and does not work on windows (it prints zone name, not offset).
	 * Calculate time zone offset by comparing local and utc time_t values for the same time.
	 */
	gmtime_r(&utc, &tm_utc);
	tm_utc.tm_isdst = tm_local.tm_isdst;
	local = mktime(&tm_utc);
	offset = static_cast<time_t>(difftime(utc, local));
	if(offset == 0) {
		strcat(buf, "Z");
	}
	else {
		if(offset > 0) {
			strcat(buf, "+");
		}
		else {
			strcat(buf, "-");
			offset = -offset;
		}
		p = buf + strlen(buf);
		buf_size = sizeof(buf) - strlen(buf);
		snprintf(p, buf_size, "%02d'%02d", (int)(offset/3600), (int)(offset%3600)/60);
	}
	strcat(buf, ")");
	ic->docinfo.create_date = sstrdup(buf);
}

cairo_int_status_t _cairo_pdf_interchange_init(cairo_pdf_surface_t * surface)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_pdf_outline_entry_t * outline_root;
	cairo_int_status_t status;
	_cairo_tag_stack_init(&ic->analysis_tag_stack);
	_cairo_tag_stack_init(&ic->render_tag_stack);
	_cairo_array_init(&ic->push_data, sizeof(void *));
	ic->struct_root = (cairo_pdf_struct_tree_node_t *)SAlloc::C(1, sizeof(cairo_pdf_struct_tree_node_t));
	if(UNLIKELY(ic->struct_root == NULL))
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	cairo_list_init(&ic->struct_root->children);
	_cairo_array_init(&ic->struct_root->mcid, sizeof(struct page_mcid));
	ic->current_node = ic->struct_root;
	ic->begin_page_node = NULL;
	ic->end_page_node = NULL;
	_cairo_array_init(&ic->parent_tree, sizeof(cairo_pdf_resource_t));
	_cairo_array_init(&ic->mcid_to_tree, sizeof(cairo_pdf_struct_tree_node_t *));
	_cairo_array_init(&ic->annots, sizeof(cairo_pdf_annotation_t *));
	ic->parent_tree_res.id = 0;
	cairo_list_init(&ic->extents_list);
	ic->named_dests = _cairo_hash_table_create(_named_dest_equal);
	if(UNLIKELY(ic->named_dests == NULL))
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	ic->num_dests = 0;
	ic->sorted_dests = NULL;
	ic->dests_res.id = 0;
	_cairo_array_init(&ic->outline, sizeof(cairo_pdf_outline_entry_t *));
	outline_root = (cairo_pdf_outline_entry_t *)SAlloc::C(1, sizeof(cairo_pdf_outline_entry_t));
	if(UNLIKELY(outline_root == NULL))
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	memzero(&ic->docinfo, sizeof(ic->docinfo));
	_cairo_pdf_interchange_set_create_date(surface);
	status = _cairo_array_append(&ic->outline, &outline_root);
	return status;
}

static void _cairo_pdf_interchange_free_outlines(cairo_pdf_surface_t * surface)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;
	int num_elems = _cairo_array_num_elements(&ic->outline);
	for(int i = 0; i < num_elems; i++) {
		cairo_pdf_outline_entry_t * outline;
		_cairo_array_copy_element(&ic->outline, i, &outline);
		SAlloc::F(outline->name);
		SAlloc::F(outline->link_attrs.dest);
		SAlloc::F(outline->link_attrs.uri);
		SAlloc::F(outline->link_attrs.file);
		SAlloc::F(outline);
	}
	_cairo_array_fini(&ic->outline);
}

cairo_int_status_t _cairo_pdf_interchange_fini(cairo_pdf_surface_t * surface)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;
	_cairo_tag_stack_fini(&ic->analysis_tag_stack);
	_cairo_tag_stack_fini(&ic->render_tag_stack);
	_cairo_array_fini(&ic->push_data);
	free_node(ic->struct_root);
	_cairo_array_fini(&ic->mcid_to_tree);
	cairo_pdf_interchange_clear_annotations(surface);
	_cairo_array_fini(&ic->annots);
	_cairo_array_fini(&ic->parent_tree);
	_cairo_hash_table_foreach(ic->named_dests, _named_dest_pluck, ic->named_dests);
	_cairo_hash_table_destroy(ic->named_dests);
	SAlloc::F(ic->sorted_dests);
	_cairo_pdf_interchange_free_outlines(surface);
	SAlloc::F(ic->docinfo.title);
	SAlloc::F(ic->docinfo.author);
	SAlloc::F(ic->docinfo.subject);
	SAlloc::F(ic->docinfo.keywords);
	SAlloc::F(ic->docinfo.creator);
	SAlloc::F(ic->docinfo.create_date);
	SAlloc::F(ic->docinfo.mod_date);
	return CAIRO_STATUS_SUCCESS;
}

cairo_int_status_t _cairo_pdf_interchange_add_outline(cairo_pdf_surface_t * surface, int parent_id,
    const char * name, const char * link_attribs, cairo_pdf_outline_flags_t flags, int * id)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_pdf_outline_entry_t * outline;
	cairo_pdf_outline_entry_t * parent;
	cairo_int_status_t status;
	if(parent_id < 0 || parent_id >= (int)_cairo_array_num_elements(&ic->outline))
		return CAIRO_STATUS_SUCCESS;
	outline = (cairo_pdf_outline_entry_t *)SAlloc::M_zon0(sizeof(cairo_pdf_outline_entry_t));
	if(UNLIKELY(outline == NULL))
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	status = _cairo_tag_parse_link_attributes(link_attribs, &outline->link_attrs);
	if(UNLIKELY(status)) {
		SAlloc::F(outline);
		return status;
	}
	outline->res = _cairo_pdf_surface_new_object(surface);
	if(outline->res.id == 0)
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	outline->name = sstrdup(name);
	outline->flags = flags;
	outline->count = 0;
	_cairo_array_copy_element(&ic->outline, parent_id, &parent);
	outline->parent = parent;
	outline->first_child = NULL;
	outline->last_child = NULL;
	outline->next = NULL;
	if(parent->last_child) {
		parent->last_child->next = outline;
		outline->prev = parent->last_child;
		parent->last_child = outline;
	}
	else {
		parent->first_child = outline;
		parent->last_child = outline;
		outline->prev = NULL;
	}
	*id = _cairo_array_num_elements(&ic->outline);
	status = _cairo_array_append(&ic->outline, &outline);
	if(UNLIKELY(status))
		return status;
	/* Update Count */
	outline = outline->parent;
	while(outline) {
		if(outline->flags & CAIRO_PDF_OUTLINE_FLAG_OPEN) {
			outline->count++;
		}
		else {
			outline->count--;
			break;
		}
		outline = outline->parent;
	}

	return CAIRO_STATUS_SUCCESS;
}

/*
 * Date must be in the following format:
 *
 *     YYYY-MM-DDThh:mm:ss[Z+-]hh:mm
 *
 * Only the year is required. If a field is included all preceding
 * fields must be included.
 */
static char * iso8601_to_pdf_date_string(const char * iso)
{
	char buf[40];
	int i;
	/* Check that utf8 contains only the characters "0123456789-T:Z+" */
	const char * p = iso;
	while(*p) {
		if(!isdec(*p) && *p != '-' && *p != 'T' && *p != ':' && *p != 'Z' && *p != '+')
			return NULL;
		p++;
	}
	p = iso;
	strcpy(buf, "(");
	/* YYYY (required) */
	if(strlen(p) < 4)
		return NULL;
	strncat(buf, p, 4);
	p += 4;
	/* -MM, -DD, Thh, :mm, :ss */
	for(i = 0; i < 5; i++) {
		if(strlen(p) < 3)
			goto finish;

		strncat(buf, p + 1, 2);
		p += 3;
	}
	/* Z, +, - */
	if(strlen(p) < 1)
		goto finish;
	strncat(buf, p, 1);
	p += 1;
	/* hh */
	if(strlen(p) < 2)
		goto finish;
	strncat(buf, p, 2);
	strcat(buf, "'");
	p += 2;
	/* :mm */
	if(strlen(p) < 3)
		goto finish;
	strncat(buf, p + 1, 3);
finish:
	strcat(buf, ")");
	return sstrdup(buf);
}

cairo_int_status_t _cairo_pdf_interchange_set_metadata(cairo_pdf_surface_t  * surface, cairo_pdf_metadata_t metadata, const char * utf8)
{
	cairo_pdf_interchange_t * ic = &surface->interchange;
	cairo_status_t status;
	char * s = NULL;
	if(utf8) {
		if(metadata == CAIRO_PDF_METADATA_CREATE_DATE ||
		    metadata == CAIRO_PDF_METADATA_MOD_DATE) {
			s = iso8601_to_pdf_date_string(utf8);
		}
		else {
			status = _cairo_utf8_to_pdf_string(utf8, &s);
			if(UNLIKELY(status))
				return status;
		}
	}
	switch(metadata) {
		case CAIRO_PDF_METADATA_TITLE:
		    SAlloc::F(ic->docinfo.title);
		    ic->docinfo.title = s;
		    break;
		case CAIRO_PDF_METADATA_AUTHOR:
		    SAlloc::F(ic->docinfo.author);
		    ic->docinfo.author = s;
		    break;
		case CAIRO_PDF_METADATA_SUBJECT:
		    SAlloc::F(ic->docinfo.subject);
		    ic->docinfo.subject = s;
		    break;
		case CAIRO_PDF_METADATA_KEYWORDS:
		    SAlloc::F(ic->docinfo.keywords);
		    ic->docinfo.keywords = s;
		    break;
		case CAIRO_PDF_METADATA_CREATOR:
		    SAlloc::F(ic->docinfo.creator);
		    ic->docinfo.creator = s;
		    break;
		case CAIRO_PDF_METADATA_CREATE_DATE:
		    SAlloc::F(ic->docinfo.create_date);
		    ic->docinfo.create_date = s;
		    break;
		case CAIRO_PDF_METADATA_MOD_DATE:
		    SAlloc::F(ic->docinfo.mod_date);
		    ic->docinfo.mod_date = s;
		    break;
	}
	return CAIRO_STATUS_SUCCESS;
}
