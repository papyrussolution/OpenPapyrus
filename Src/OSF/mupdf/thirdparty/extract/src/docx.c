/* These extract_docx_*() functions generate docx content and docx zip archive
   data.

   Caller must call things in a sensible order to create valid content -
   e.g. don't call docx_paragraph_start() twice without intervening call to
   docx_paragraph_finish(). */

#include "../include/extract.h"

#include "docx_template.h"

#include "alloc.h"
#include "astring.h"
#include "document.h"
#include "docx.h"
#include "mem.h"
#include "memento.h"
#include "outf.h"
#include "zip.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>

static int extract_docx_paragraph_start(extract_astring_t* content)
{
	return extract_astring_cat(content, "\n\n<w:p>");
}

static int extract_docx_paragraph_finish(extract_astring_t* content)
{
	return extract_astring_cat(content, "\n</w:p>");
}

static int extract_docx_run_start(extract_astring_t* content,
    const char* font_name,
    double font_size,
    int bold,
    int italic
    )
/* Starts a new run. Caller must ensure that extract_docx_run_finish() was
   called to terminate any previous run. */
{
	int e = 0;
	if(!e) e = extract_astring_cat(content, "\n<w:r><w:rPr><w:rFonts w:ascii=\"");
	if(!e) e = extract_astring_cat(content, font_name);
	if(!e) e = extract_astring_cat(content, "\" w:hAnsi=\"");
	if(!e) e = extract_astring_cat(content, font_name);
	if(!e) e = extract_astring_cat(content, "\"/>");
	if(!e && bold) e = extract_astring_cat(content, "<w:b/>");
	if(!e && italic) e = extract_astring_cat(content, "<w:i/>");
	{
		char font_size_text[32];
		if(0) font_size = 10;

		if(!e) e = extract_astring_cat(content, "<w:sz w:val=\"");
		snprintf(font_size_text, sizeof(font_size_text), "%f", font_size * 2);
		extract_astring_cat(content, font_size_text);
		extract_astring_cat(content, "\"/>");

		if(!e) e = extract_astring_cat(content, "<w:szCs w:val=\"");
		snprintf(font_size_text, sizeof(font_size_text), "%f", font_size * 1.5);
		extract_astring_cat(content, font_size_text);
		extract_astring_cat(content, "\"/>");
	}
	if(!e) e = extract_astring_cat(content, "</w:rPr><w:t xml:space=\"preserve\">");
	return e;
}

static int extract_docx_run_finish(extract_astring_t* content)
{
	return extract_astring_cat(content, "</w:t></w:r>");
}

static int extract_docx_char_append_string(extract_astring_t* content, const char* text)
{
	return extract_astring_cat(content, text);
}

static int extract_docx_char_append_stringf(extract_astring_t* content, const char* format, ...)
{
	char* buffer = NULL;
	int e;
	va_list va;
	va_start(va, format);
	e = extract_vasprintf(&buffer, format, va);
	va_end(va);
	if(e < 0) return e;
	e = extract_astring_cat(content, buffer);
	extract_free(&buffer);
	return e;
}

static int extract_docx_char_append_char(extract_astring_t* content, char c)
{
	return extract_astring_catc(content, c);
}

static int extract_docx_paragraph_empty(extract_astring_t* content)
/* Append an empty paragraph to *content. */
{
	int e = -1;
	if(extract_docx_paragraph_start(content)) goto end;
	/* It seems like our choice of font size here doesn't make any difference
	   to the ammount of vertical space, unless we include a non-space
	   character. Presumably something to do with the styles in the template
	   document. */
	if(extract_docx_run_start(
		    content,
		    "OpenSans",
		    10 /*font_size*/,
		    0 /*font_bold*/,
		    0 /*font_italic*/
		    )) goto end;
	//docx_char_append_string(content, "&#160;");   /* &#160; is non-break space. */
	if(extract_docx_run_finish(content)) goto end;
	if(extract_docx_paragraph_finish(content)) goto end;
	e = 0;
end:
	return e;
}

/* Removes last <len> chars. */
static int docx_char_truncate(extract_astring_t* content, int len)
{
	assert((size_t)len <= content->chars_num);
	content->chars_num -= len;
	content->chars[content->chars_num] = 0;
	return 0;
}

static int extract_docx_char_truncate_if(extract_astring_t* content, char c)
/* Removes last char if it is <c>. */
{
	if(content->chars_num && content->chars[content->chars_num-1] == c) {
		docx_char_truncate(content, 1);
	}
	return 0;
}

static double matrices_to_font_size(matrix_t* ctm, matrix_t* trm)
{
	double font_size = matrix_expansion(*trm)
	    * matrix_expansion(*ctm);
	/* Round font_size to nearest 0.01. */
	font_size = (double)(int)(font_size * 100.0f + 0.5f) / 100.0f;
	return font_size;
}

typedef struct {
	const char* font_name;
	double font_size;
	int font_bold;
	int font_italic;
	matrix_t*   ctm_prev;
} content_state_t;
/* Used to keep track of font information when writing paragraphs of docx
   content, e.g. so we know whether a font has changed so need to start a new docx
   span. */

static int extract_document_to_docx_content_paragraph(content_state_t*    state,
    paragraph_t*        paragraph,
    extract_astring_t*  content
    )
/* Append docx xml for <paragraph> to <content>. Updates *state if we change
   font. */
{
	int e = -1;
	int l;
	if(extract_docx_paragraph_start(content)) goto end;

	for(l = 0; l<paragraph->lines_num; ++l) {
		line_t* line = paragraph->lines[l];
		int s;
		for(s = 0; s<line->spans_num; ++s) {
			int si;
			span_t* span = line->spans[s];
			double font_size_new;
			state->ctm_prev = &span->ctm;
			font_size_new = matrices_to_font_size(&span->ctm, &span->trm);
			if(!state->font_name
			    || strcmp(span->font_name, state->font_name)
			    || span->font_bold != state->font_bold
			    || span->font_italic != state->font_italic
			    || font_size_new != state->font_size
			    ) {
				if(state->font_name) {
					if(extract_docx_run_finish(content)) goto end;
				}
				state->font_name = span->font_name;
				state->font_bold = span->font_bold;
				state->font_italic = span->font_italic;
				state->font_size = font_size_new;
				if(extract_docx_run_start(
					    content,
					    state->font_name,
					    state->font_size,
					    state->font_bold,
					    state->font_italic
					    )) goto end;
			}

			for(si = 0; si<span->chars_num; ++si) {
				char_t* char_ = &span->chars[si];
				int c = char_->ucs;

				if(0) {
				}

				/* Escape XML special characters. */
				else if(c == '<') extract_docx_char_append_string(content, "&lt;");
				else if(c == '>') extract_docx_char_append_string(content, "&gt;");
				else if(c == '&') extract_docx_char_append_string(content, "&amp;");
				else if(c == '"') extract_docx_char_append_string(content, "&quot;");
				else if(c == '\'') extract_docx_char_append_string(content, "&apos;");

				/* Expand ligatures. */
				else if(c == 0xFB00) {
					if(extract_docx_char_append_string(content, "ff")) goto end;
				}
				else if(c == 0xFB01) {
					if(extract_docx_char_append_string(content, "fi")) goto end;
				}
				else if(c == 0xFB02) {
					if(extract_docx_char_append_string(content, "fl")) goto end;
				}
				else if(c == 0xFB03) {
					if(extract_docx_char_append_string(content, "ffi")) goto end;
				}
				else if(c == 0xFB04) {
					if(extract_docx_char_append_string(content, "ffl")) goto end;
				}

				/* Output ASCII verbatim. */
				else if(c >= 32 && c <= 127) {
					if(extract_docx_char_append_char(content, (char)c)) goto end;
				}

				/* Escape all other characters. */
				else {
					char buffer[32];
					snprintf(buffer, sizeof(buffer), "&#x%x;", c);
					if(extract_docx_char_append_string(content, buffer)) goto end;
				}
			}
			/* Remove any trailing '-' at end of line. */
			if(extract_docx_char_truncate_if(content, '-')) goto end;
		}
	}
	if(state->font_name) {
		if(extract_docx_run_finish(content)) goto end;
		state->font_name = NULL;
	}
	if(extract_docx_paragraph_finish(content)) goto end;

	e = 0;

end:
	return e;
}

static int extract_document_append_image(extract_astring_t*  content,
    image_t*            image
    )
/* Write reference to image into docx content. */
{
	extract_docx_char_append_string(content, "\n");
	extract_docx_char_append_string(content, "     <w:p>\n");
	extract_docx_char_append_string(content, "       <w:r>\n");
	extract_docx_char_append_string(content, "         <w:rPr>\n");
	extract_docx_char_append_string(content, "           <w:noProof/>\n");
	extract_docx_char_append_string(content, "         </w:rPr>\n");
	extract_docx_char_append_string(content, "         <w:drawing>\n");
	extract_docx_char_append_string(content,
	    "           <wp:inline distT=\"0\" distB=\"0\" distL=\"0\" distR=\"0\" wp14:anchorId=\"7057A832\" wp14:editId=\"466EB3FB\">\n");
	extract_docx_char_append_string(content, "             <wp:extent cx=\"2933700\" cy=\"2200275\"/>\n");
	extract_docx_char_append_string(content, "             <wp:effectExtent l=\"0\" t=\"0\" r=\"0\" b=\"9525\"/>\n");
	extract_docx_char_append_string(content, "             <wp:docPr id=\"1\" name=\"Picture 1\"/>\n");
	extract_docx_char_append_string(content, "             <wp:cNvGraphicFramePr>\n");
	extract_docx_char_append_string(content,
	    "               <a:graphicFrameLocks xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" noChangeAspect=\"1\"/>\n");
	extract_docx_char_append_string(content, "             </wp:cNvGraphicFramePr>\n");
	extract_docx_char_append_string(content,
	    "             <a:graphic xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\">\n");
	extract_docx_char_append_string(content,
	    "               <a:graphicData uri=\"http://schemas.openxmlformats.org/drawingml/2006/picture\">\n");
	extract_docx_char_append_string(content,
	    "                 <pic:pic xmlns:pic=\"http://schemas.openxmlformats.org/drawingml/2006/picture\">\n");
	extract_docx_char_append_string(content, "                   <pic:nvPicPr>\n");
	extract_docx_char_append_string(content, "                     <pic:cNvPr id=\"1\" name=\"Picture 1\"/>\n");
	extract_docx_char_append_string(content, "                     <pic:cNvPicPr>\n");
	extract_docx_char_append_string(content, "                       <a:picLocks noChangeAspect=\"1\" noChangeArrowheads=\"1\"/>\n");
	extract_docx_char_append_string(content, "                     </pic:cNvPicPr>\n");
	extract_docx_char_append_string(content, "                   </pic:nvPicPr>\n");
	extract_docx_char_append_string(content, "                   <pic:blipFill>\n");
	extract_docx_char_append_stringf(content, "                     <a:blip r:embed=\"%s\">\n", image->id);
	extract_docx_char_append_string(content, "                       <a:extLst>\n");
	extract_docx_char_append_string(content, "                         <a:ext uri=\"{28A0092B-C50C-407E-A947-70E740481C1C}\">\n");
	extract_docx_char_append_string(content,
	    "                           <a14:useLocalDpi xmlns:a14=\"http://schemas.microsoft.com/office/drawing/2010/main\" val=\"0\"/>\n");
	extract_docx_char_append_string(content, "                         </a:ext>\n");
	extract_docx_char_append_string(content, "                       </a:extLst>\n");
	extract_docx_char_append_string(content, "                     </a:blip>\n");
	//extract_docx_char_append_string(content, "                     <a:srcRect/>\n");
	extract_docx_char_append_string(content, "                     <a:stretch>\n");
	extract_docx_char_append_string(content, "                       <a:fillRect/>\n");
	extract_docx_char_append_string(content, "                     </a:stretch>\n");
	extract_docx_char_append_string(content, "                   </pic:blipFill>\n");
	extract_docx_char_append_string(content, "                   <pic:spPr bwMode=\"auto\">\n");
	extract_docx_char_append_string(content, "                     <a:xfrm>\n");
	extract_docx_char_append_string(content, "                       <a:off x=\"0\" y=\"0\"/>\n");
	extract_docx_char_append_string(content, "                       <a:ext cx=\"2933700\" cy=\"2200275\"/>\n");
	extract_docx_char_append_string(content, "                     </a:xfrm>\n");
	extract_docx_char_append_string(content, "                     <a:prstGeom prst=\"rect\">\n");
	extract_docx_char_append_string(content, "                       <a:avLst/>\n");
	extract_docx_char_append_string(content, "                     </a:prstGeom>\n");
	extract_docx_char_append_string(content, "                     <a:noFill/>\n");
	extract_docx_char_append_string(content, "                     <a:ln>\n");
	extract_docx_char_append_string(content, "                       <a:noFill/>\n");
	extract_docx_char_append_string(content, "                     </a:ln>\n");
	extract_docx_char_append_string(content, "                   </pic:spPr>\n");
	extract_docx_char_append_string(content, "                 </pic:pic>\n");
	extract_docx_char_append_string(content, "               </a:graphicData>\n");
	extract_docx_char_append_string(content, "             </a:graphic>\n");
	extract_docx_char_append_string(content, "           </wp:inline>\n");
	extract_docx_char_append_string(content, "         </w:drawing>\n");
	extract_docx_char_append_string(content, "       </w:r>\n");
	extract_docx_char_append_string(content, "     </w:p>\n");
	extract_docx_char_append_string(content, "\n");
	return 0;
}

static int extract_document_output_rotated_paragraphs(page_t*             page,
    int paragraph_begin,
    int paragraph_end,
    int rot,
    int x,
    int y,
    int w,
    int h,
    int text_box_id,
    extract_astring_t*  content,
    content_state_t*    state
    )
/* Writes paragraph to content inside rotated text box. */
{
	int e = 0;
	int p;
	outf("x,y=%ik,%ik = %i,%i", x/1000, y/1000, x, y);
	extract_docx_char_append_string(content, "\n");
	extract_docx_char_append_string(content, "\n");
	extract_docx_char_append_string(content, "<w:p>\n");
	extract_docx_char_append_string(content, "  <w:r>\n");
	extract_docx_char_append_string(content, "    <mc:AlternateContent>\n");
	extract_docx_char_append_string(content, "      <mc:Choice Requires=\"wps\">\n");
	extract_docx_char_append_string(content, "        <w:drawing>\n");
	extract_docx_char_append_string(content,
	    "          <wp:anchor distT=\"0\" distB=\"0\" distL=\"0\" distR=\"0\" simplePos=\"0\" relativeHeight=\"0\" behindDoc=\"0\" locked=\"0\" layoutInCell=\"1\" allowOverlap=\"1\" wp14:anchorId=\"53A210D1\" wp14:editId=\"2B7E8016\">\n");
	extract_docx_char_append_string(content, "            <wp:simplePos x=\"0\" y=\"0\"/>\n");
	extract_docx_char_append_string(content, "            <wp:positionH relativeFrom=\"page\">\n");
	extract_docx_char_append_stringf(content, "              <wp:posOffset>%i</wp:posOffset>\n", x);
	extract_docx_char_append_string(content, "            </wp:positionH>\n");
	extract_docx_char_append_string(content, "            <wp:positionV relativeFrom=\"page\">\n");
	extract_docx_char_append_stringf(content, "              <wp:posOffset>%i</wp:posOffset>\n", y);
	extract_docx_char_append_string(content, "            </wp:positionV>\n");
	extract_docx_char_append_stringf(content, "            <wp:extent cx=\"%i\" cy=\"%i\"/>\n", w, h);
	extract_docx_char_append_string(content, "            <wp:effectExtent l=\"381000\" t=\"723900\" r=\"371475\" b=\"723900\"/>\n");
	extract_docx_char_append_string(content, "            <wp:wrapNone/>\n");
	extract_docx_char_append_stringf(content, "            <wp:docPr id=\"%i\" name=\"Text Box %i\"/>\n", text_box_id, text_box_id);
	extract_docx_char_append_string(content, "            <wp:cNvGraphicFramePr/>\n");
	extract_docx_char_append_string(content,
	    "            <a:graphic xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\">\n");
	extract_docx_char_append_string(content,
	    "              <a:graphicData uri=\"http://schemas.microsoft.com/office/word/2010/wordprocessingShape\">\n");
	extract_docx_char_append_string(content, "                <wps:wsp>\n");
	extract_docx_char_append_string(content, "                  <wps:cNvSpPr txBox=\"1\"/>\n");
	extract_docx_char_append_string(content, "                  <wps:spPr>\n");
	extract_docx_char_append_stringf(content, "                    <a:xfrm rot=\"%i\">\n", rot);
	extract_docx_char_append_string(content, "                      <a:off x=\"0\" y=\"0\"/>\n");
	extract_docx_char_append_string(content, "                      <a:ext cx=\"3228975\" cy=\"2286000\"/>\n");
	extract_docx_char_append_string(content, "                    </a:xfrm>\n");
	extract_docx_char_append_string(content, "                    <a:prstGeom prst=\"rect\">\n");
	extract_docx_char_append_string(content, "                      <a:avLst/>\n");
	extract_docx_char_append_string(content, "                    </a:prstGeom>\n");

	/* Give box a solid background. */
	if(0) {
		extract_docx_char_append_string(content, "                    <a:solidFill>\n");
		extract_docx_char_append_string(content, "                      <a:schemeClr val=\"lt1\"/>\n");
		extract_docx_char_append_string(content, "                    </a:solidFill>\n");
	}

	/* Draw line around box. */
	if(0) {
		extract_docx_char_append_string(content, "                    <a:ln w=\"175\">\n");
		extract_docx_char_append_string(content, "                      <a:solidFill>\n");
		extract_docx_char_append_string(content, "                        <a:prstClr val=\"black\"/>\n");
		extract_docx_char_append_string(content, "                      </a:solidFill>\n");
		extract_docx_char_append_string(content, "                    </a:ln>\n");
	}

	extract_docx_char_append_string(content, "                  </wps:spPr>\n");
	extract_docx_char_append_string(content, "                  <wps:txbx>\n");
	extract_docx_char_append_string(content, "                    <w:txbxContent>");

    #if 0
	if(0) {
		/* Output inline text describing the rotation. */
		extract_docx_char_append_stringf(content,
		    "<w:p>\n"
		    "<w:r><w:rPr><w:rFonts w:ascii=\"OpenSans\" w:hAnsi=\"OpenSans\"/><w:sz w:val=\"20.000000\"/><w:szCs w:val=\"15.000000\"/></w:rPr><w:t xml:space=\"preserve\">*** rotate: %f rad, %f deg. rot=%i</w:t></w:r>\n"
		    "</w:p>\n",
		    rotate,
		    rotate * 180 / pi,
		    rot
		    );
	}
    #endif

	/* Output paragraphs p0..p2-1. */
	for(p = paragraph_begin; p<paragraph_end; ++p) {
		paragraph_t* paragraph = page->paragraphs[p];
		if(extract_document_to_docx_content_paragraph(state, paragraph, content)) goto end;
	}

	extract_docx_char_append_string(content, "\n");
	extract_docx_char_append_string(content, "                    </w:txbxContent>\n");
	extract_docx_char_append_string(content, "                  </wps:txbx>\n");
	extract_docx_char_append_string(content,
	    "                  <wps:bodyPr rot=\"0\" spcFirstLastPara=\"0\" vertOverflow=\"overflow\" horzOverflow=\"overflow\" vert=\"horz\" wrap=\"square\" lIns=\"91440\" tIns=\"45720\" rIns=\"91440\" bIns=\"45720\" numCol=\"1\" spcCol=\"0\" rtlCol=\"0\" fromWordArt=\"0\" anchor=\"t\" anchorCtr=\"0\" forceAA=\"0\" compatLnSpc=\"1\">\n");
	extract_docx_char_append_string(content, "                    <a:prstTxWarp prst=\"textNoShape\">\n");
	extract_docx_char_append_string(content, "                      <a:avLst/>\n");
	extract_docx_char_append_string(content, "                    </a:prstTxWarp>\n");
	extract_docx_char_append_string(content, "                    <a:noAutofit/>\n");
	extract_docx_char_append_string(content, "                  </wps:bodyPr>\n");
	extract_docx_char_append_string(content, "                </wps:wsp>\n");
	extract_docx_char_append_string(content, "              </a:graphicData>\n");
	extract_docx_char_append_string(content, "            </a:graphic>\n");
	extract_docx_char_append_string(content, "          </wp:anchor>\n");
	extract_docx_char_append_string(content, "        </w:drawing>\n");
	extract_docx_char_append_string(content, "      </mc:Choice>\n");

	/* This fallack is copied from a real Word document. Not sure
	   whether it works - both Libreoffice and Word use the above
	   choice. */
	extract_docx_char_append_string(content, "      <mc:Fallback>\n");
	extract_docx_char_append_string(content, "        <w:pict>\n");
	extract_docx_char_append_string(content,
	    "          <v:shapetype w14:anchorId=\"53A210D1\" id=\"_x0000_t202\" coordsize=\"21600,21600\" o:spt=\"202\" path=\"m,l,21600r21600,l21600,xe\">\n");
	extract_docx_char_append_string(content, "            <v:stroke joinstyle=\"miter\"/>\n");
	extract_docx_char_append_string(content, "            <v:path gradientshapeok=\"t\" o:connecttype=\"rect\"/>\n");
	extract_docx_char_append_string(content, "          </v:shapetype>\n");
	extract_docx_char_append_stringf(content,
	    "          <v:shape id=\"Text Box %i\" o:spid=\"_x0000_s1026\" type=\"#_x0000_t202\" style=\"position:absolute;margin-left:71.25pt;margin-top:48.75pt;width:254.25pt;height:180pt;rotation:-2241476fd;z-index:251659264;visibility:visible;mso-wrap-style:square;mso-wrap-distance-left:9pt;mso-wrap-distance-top:0;mso-wrap-distance-right:9pt;mso-wrap-distance-bottom:0;mso-position-horizontal:absolute;mso-position-horizontal-relative:text;mso-position-vertical:absolute;mso-position-vertical-relative:text;v-text-anchor:top\" o:gfxdata=\"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&#10;bFBLBQYAAAAABAAEAPMAAAC8BQAAAAA=&#10;\" fillcolor=\"white [3201]\" strokeweight=\".5pt\">\n",
	    text_box_id);
	extract_docx_char_append_string(content, "            <v:textbox>\n");
	extract_docx_char_append_string(content, "              <w:txbxContent>");

	for(p = paragraph_begin; p<paragraph_end; ++p) {
		paragraph_t* paragraph = page->paragraphs[p];
		if(extract_document_to_docx_content_paragraph(state, paragraph, content)) goto end;
	}

	extract_docx_char_append_string(content, "\n");
	extract_docx_char_append_string(content, "\n");
	extract_docx_char_append_string(content, "              </w:txbxContent>\n");
	extract_docx_char_append_string(content, "            </v:textbox>\n");
	extract_docx_char_append_string(content, "          </v:shape>\n");
	extract_docx_char_append_string(content, "        </w:pict>\n");
	extract_docx_char_append_string(content, "      </mc:Fallback>\n");
	extract_docx_char_append_string(content, "    </mc:AlternateContent>\n");
	extract_docx_char_append_string(content, "  </w:r>\n");
	extract_docx_char_append_string(content, "</w:p>");
	e = 0;
end:
	return e;
}

int extract_document_to_docx_content(document_t*         document,
    int spacing,
    int rotation,
    int images,
    extract_astring_t*  content
    )
{
	int ret = -1;
	int text_box_id = 0;
	int p;

	/* Write paragraphs into <content>. */
	for(p = 0; p<document->pages_num; ++p) {
		page_t* page = document->pages[p];
		int p;
		content_state_t state;
		state.font_name = NULL;
		state.font_size = 0;
		state.font_bold = 0;
		state.font_italic = 0;
		state.ctm_prev = NULL;

		for(p = 0; p<page->paragraphs_num; ++p) {
			paragraph_t* paragraph = page->paragraphs[p];
			const matrix_t* ctm = &paragraph->lines[0]->spans[0]->ctm;
			double rotate = atan2(ctm->b, ctm->a);

			if(spacing
			    && state.ctm_prev
			    && paragraph->lines_num
			    && paragraph->lines[0]->spans_num
			    && matrix_cmp4(
				    state.ctm_prev,
				    &paragraph->lines[0]->spans[0]->ctm
				    )
			    ) {
				/* Extra vertical space between paragraphs that were at
				   different angles in the original document. */
				if(extract_docx_paragraph_empty(content)) goto end;
			}

			if(spacing) {
				/* Extra vertical space between paragraphs. */
				if(extract_docx_paragraph_empty(content)) goto end;
			}

			if(rotation && rotate != 0) {
				/* Find extent of paragraphs with this same rotation. extent
				   will contain max width and max height of paragraphs, in units
				   before application of ctm, i.e. before rotation. */
				point_t extent = {0, 0};
				int p0 = p;
				int p1;

				outf("rotate=%.2frad=%.1fdeg ctm: ef=(%f %f) abcd=(%f %f %f %f)",
				    rotate, rotate * 180 / pi,
				    ctm->e,
				    ctm->f,
				    ctm->a,
				    ctm->b,
				    ctm->c,
				    ctm->d
				    );

				{
					/* We assume that first span is at origin of text
					   block. This assumes left-to-right text. */
					double rotate0 = rotate;
					const matrix_t* ctm0 = ctm;
					point_t origin = {
						paragraph->lines[0]->spans[0]->chars[0].x,
						paragraph->lines[0]->spans[0]->chars[0].y
					};
					matrix_t ctm_inverse = {1, 0, 0, 1, 0, 0};
					double ctm_det = ctm->a*ctm->d - ctm->b*ctm->c;
					if(ctm_det != 0) {
						ctm_inverse.a = +ctm->d / ctm_det;
						ctm_inverse.b = -ctm->b / ctm_det;
						ctm_inverse.c = -ctm->c / ctm_det;
						ctm_inverse.d = +ctm->a / ctm_det;
					}
					else {
						outf("cannot invert ctm=(%f %f %f %f)",
						    ctm->a, ctm->b, ctm->c, ctm->d);
					}

					for(p = p0; p<page->paragraphs_num; ++p) {
						paragraph = page->paragraphs[p];
						ctm = &paragraph->lines[0]->spans[0]->ctm;
						rotate = atan2(ctm->b, ctm->a);
						if(rotate != rotate0) {
							break;
						}

						/* Update <extent>. */
						{
							int l;
							for(l = 0; l<paragraph->lines_num; ++l) {
								line_t* line = paragraph->lines[l];
								span_t* span = line_span_last(line);
								char_t* char_ = span_char_last(span);
								double adv = char_->adv * matrix_expansion(span->trm);
								double x = char_->x + adv * cos(rotate);
								double y = char_->y + adv * sin(rotate);

								double dx = x - origin.x;
								double dy = y - origin.y;

								/* Position relative to origin and before box rotation.
								   */
								double xx = ctm_inverse.a * dx + ctm_inverse.b * dy;
								double yy = ctm_inverse.c * dx + ctm_inverse.d * dy;
								yy = -yy;
								if(xx > extent.x) extent.x = xx;
								if(yy > extent.y) extent.y = yy;
								if(0) outf(
										"rotate=%f p=%i: origin=(%f %f) xy=(%f %f) dxy=(%f %f) xxyy=(%f %f) span: %s",
										rotate,
										p,
										origin.x,
										origin.y,
										x,
										y,
										dx,
										dy,
										xx,
										yy,
										span_string(span));
							}
						}
					}
					p1 = p;
					rotate = rotate0;
					ctm = ctm0;
					outf("rotate=%f p0=%i p1=%i. extent is: (%f %f)",
					    rotate, p0, p1, extent.x, extent.y);
				}

				/* Paragraphs p0..p1-1 have same rotation. We output them into
				   a single rotated text box. */

				/* We need unique id for text box. */
				text_box_id += 1;

				{
					/* Angles are in units of 1/60,000 degree. */
					int rot = (int)(rotate * 180 / pi * 60000);

					/* <wp:anchor distT=\.. etc are in EMU - 1/360,000 of a cm.
					   relativeHeight is z-ordering. (wp:positionV:wp:posOffset,
					   wp:positionV:wp:posOffset) is position of origin of box in
					   EMU.

					   The box rotates about its centre but we want to rotate
					   about the origin (top-left). So we correct the position of
					   box by subtracting the vector that the top-left moves when
					   rotated by angle <rotate> about the middle. */
					double point_to_emu = 12700; /*
					                                https://en.wikipedia.org/wiki/Office_Open_XML_file_formats#DrawingML
					                                */
					int x = (int)(ctm->e * point_to_emu);
					int y = (int)(ctm->f * point_to_emu);
					int w = (int)(extent.x * point_to_emu);
					int h = (int)(extent.y * point_to_emu);
					int dx;
					int dy;

					if(0) outf("rotate: %f rad, %f deg. rot=%i", rotate, rotate*180/pi, rot);

					h *= 2;
					/* We can't predict how much space Word will actually
					   require for the rotated text, so make the box have the
					   original width but allow text to take extra vertical
					   space. There doesn't seem to be a way to make the text box
					   auto-grow to contain the text. */

					dx = (int)((1-cos(rotate)) * w / 2.0 + sin(rotate) * h / 2.0);
					dy = (int)((cos(rotate)-1) * h / 2.0 + sin(rotate) * w / 2.0);
					outf("ctm->e,f=%f,%f rotate=%f => x,y=%ik %ik dx,dy=%ik %ik",
					    ctm->e,
					    ctm->f,
					    rotate * 180/pi,
					    x/1000,
					    y/1000,
					    dx/1000,
					    dy/1000
					    );
					x -= dx;
					y -= -dy;

					if(extract_document_output_rotated_paragraphs(page, p0, p1, rot, x, y, w, h, text_box_id, content,
					    &state)) goto end;
				}
				p = p1 - 1;
				//p = page->paragraphs_num - 1;
			}
			else {
				if(extract_document_to_docx_content_paragraph(&state, paragraph, content)) goto end;
			}
		}

		if(images) {
			int i;
			for(i = 0; i<page->images_num; ++i) {
				extract_document_append_image(content, &page->images[i]);
			}
		}
	}
	ret = 0;

end:

	return ret;
}

static int systemf(const char* format, ...)
/* Like system() but takes printf-style format and args. Also, if we return +ve
   we set errno to EIO. */
{
	int e;
	char* command;
	va_list va;
	va_start(va, format);
	e = extract_vasprintf(&command, format, va);
	va_end(va);
	if(e < 0) return e;
	outf("running: %s", command);
	e = system(command);
	extract_free(&command);
	if(e > 0) {
		errno = EIO;
	}
	return e;
}

static int read_all(FILE* in, char** o_out)
/* Reads until eof into zero-terminated malloc'd buffer. */
{
	size_t len = 0;
	size_t delta = 128;
	for(;;) {
		size_t n;
		if(extract_realloc2(o_out, len, len + delta + 1)) {
			extract_free(o_out);
			return -1;
		}
		n = fread(*o_out + len, 1 /*size*/, delta /*nmemb*/, in);
		len += n;
		if(feof(in)) {
			(*o_out)[len] = 0;
			return 0;
		}
		if(ferror(in)) {
			/* It's weird that fread() and ferror() don't set errno. */
			errno = EIO;
			extract_free(o_out);
			return -1;
		}
	}
}

static int read_all_path(const char* path, char** o_text)
/* Reads entire file into zero-terminated malloc'd buffer. */
{
	int e = -1;
	FILE * f = fopen(path, "rb");
	if(!f) goto end;
	if(read_all(f, o_text)) goto end;
	e = 0;
end:
	if(f) fclose(f);
	if(e) extract_free(&o_text);
	return e;
}

static int write_all(const void* data, size_t data_size, const char* path)
{
	int e = -1;
	FILE * f = fopen(path, "w");
	if(!f) goto end;
	if(fwrite(data, data_size, 1 /*nmemb*/, f) != 1) goto end;
	e = 0;
end:
	if(f) fclose(f);
	return e;
}

static int extract_docx_content_insert(const char*         original,
    const char*         mid_begin_name,
    const char*         mid_end_name,
    extract_astring_t*  contentss,
    int contentss_num,
    char**              o_out
    )
/* Creates a string consisting of <original> with all strings in <contentss>
   inserted into <original>'s <mid_begin_name>...<mid_end_name> region, and
   appends this string to *o_out. */
{
	int e = -1;
	const char* mid_begin;
	const char* mid_end;
	extract_astring_t out;
	extract_astring_init(&out);

	mid_begin = strstr(original, mid_begin_name);
	if(!mid_begin) {
		outf("error: could not find '%s' in docx content",
		    mid_begin_name);
		errno = ESRCH;
		goto end;
	}
	mid_begin += strlen(mid_begin_name);

	mid_end = strstr(mid_begin, mid_end_name);
	if(!mid_end) {
		outf("error: could not find '%s' in docx content",
		    mid_end_name);
		errno = ESRCH;
		goto end;
	}

	if(extract_astring_catl(&out, original, mid_begin - original)) goto end;
	{
		int i;
		for(i = 0; i<contentss_num; ++i) {
			if(extract_astring_catl(&out, contentss[i].chars, contentss[i].chars_num)) goto end;
		}
	}
	if(extract_astring_cat(&out, mid_end)) goto end;

	*o_out = out.chars;
	out.chars = NULL;
	e = 0;

end:
	if(e) {
		extract_astring_free(&out);
		*o_out = NULL;
	}
	return e;
}

static int s_find_mid(const char* text, const char* begin, const char* end, const char** o_begin, const char** o_end)
/* Sets *o_begin to end of first occurrence of <begin> in <text>, and *o_end to
   beginning of first occurtence of <end> in <text>. */
{
	*o_begin = strstr(text, begin);
	if(!*o_begin) goto fail;
	*o_begin += strlen(begin);
	*o_end = strstr(*o_begin, end);
	if(!*o_end) goto fail;
	return 0;
fail:
	errno = ESRCH;
	return -1;
}

int extract_docx_content_item(extract_astring_t*  contentss,
    int contentss_num,
    images_t*           images,
    const char*         name,
    const char*         text,
    char**              text2
    )
{
	int e = -1;
	extract_astring_t temp;
	extract_astring_init(&temp);
	*text2 = NULL;

	if(0) {
	}
	else if(!strcmp(name, "[Content_Types].xml")) {
		/* Add information about all image types that we are going to use. */
		const char* begin;
		const char* end;
		const char* insert;
		int it;
		extract_astring_free(&temp);
		outf("text: %s", text);
		if(s_find_mid(text, "<Types ", "</Types>", &begin, &end)) goto end;

		insert = begin;
		insert = strchr(insert, '>');
		assert(insert);
		insert += 1;

		if(extract_astring_catl(&temp, text, insert - text)) goto end;
		outf("images->imagetypes_num=%i", images->imagetypes_num);
		for(it = 0; it<images->imagetypes_num; ++it) {
			const char* imagetype = images->imagetypes[it];
			if(extract_astring_cat(&temp, "<Default Extension=\"")) goto end;
			if(extract_astring_cat(&temp, imagetype)) goto end;
			if(extract_astring_cat(&temp, "\" ContentType=\"image/")) goto end;
			if(extract_astring_cat(&temp, imagetype)) goto end;
			if(extract_astring_cat(&temp, "\"/>")) goto end;
		}
		if(extract_astring_cat(&temp, insert)) goto end;
		*text2 = temp.chars;
		extract_astring_init(&temp);
	}
	else if(!strcmp(name, "word/_rels/document.xml.rels")) {
		/* Add relationships between image ids and image names within docx
		   archive. */
		const char* begin;
		const char* end;
		int j;
		extract_astring_free(&temp);
		if(s_find_mid(text, "<Relationships", "</Relationships>", &begin, &end)) goto end;
		if(extract_astring_catl(&temp, text, end - text)) goto end;
		outf("images.images_num=%i", images->images_num);
		for(j = 0; j<images->images_num; ++j) {
			image_t* image = &images->images[j];
			if(extract_astring_cat(&temp, "<Relationship Id=\"")) goto end;
			if(extract_astring_cat(&temp, image->id)) goto end;
			if(extract_astring_cat(&temp,
			    "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" Target=\"media/")) goto
				end;
			if(extract_astring_cat(&temp, image->name)) goto end;
			if(extract_astring_cat(&temp, "\"/>")) goto end;
		}
		if(extract_astring_cat(&temp, end)) goto end;
		*text2 = temp.chars;
		extract_astring_init(&temp);
	}
	else if(!strcmp(name, "word/document.xml")) {
		/* Insert paragraphs content. */
		if(extract_docx_content_insert(
			    text,
			    "<w:body>",
			    "</w:body>",
			    contentss,
			    contentss_num,
			    text2
			    )) goto end;
	}
	else {
		*text2 = NULL;
	}
	e = 0;
end:
	if(e) {
		/* We might have set <text2> to new content. */
		extract_free(text2);
		/* We might have used <temp> as a temporary buffer. */
		extract_astring_free(&temp);
	}
	extract_astring_init(&temp);
	return e;
}

static int check_path_shell_safe(const char* path)
/* Returns -1 with errno=EINVAL if <path> contains sequences that could make it
   unsafe in shell commands. */
{
	if(0
	    || strstr(path, "..")
	    || strchr(path, '\'')
	    || strchr(path, '"')
	    || strchr(path, ' ')
	    ) {
		errno = EINVAL;
		return -1;
	}
	return 0;
}

static int remove_directory(const char* path)
{
	if(check_path_shell_safe(path)) {
		outf("path_out is unsafe: %s", path);
		return -1;
	}
	return systemf("rm -r '%s'", path);
}

#ifdef _WIN32
#include <direct.h>
static int s_mkdir(const char* path, int mode)
{
	(void)mode;
	return _mkdir(path);
}

#else
static int s_mkdir(const char* path, int mode)
{
	return mkdir(path, mode);
}

#endif

int extract_docx_write_template(extract_astring_t*  contentss,
    int contentss_num,
    images_t*           images,
    const char*         path_template,
    const char*         path_out,
    int preserve_dir
    )
{
	int e = -1;
	int i;
	char*   path_tempdir = NULL;
	FILE*   f = NULL;
	char*   path = NULL;
	char*   text = NULL;
	char*   text2 = NULL;

	assert(path_out);
	assert(path_template);

	if(check_path_shell_safe(path_out)) {
		outf("path_out is unsafe: %s", path_out);
		goto end;
	}

	outf("images->images_num=%i", images->images_num);
	if(extract_asprintf(&path_tempdir, "%s.dir", path_out) < 0) goto end;
	if(systemf("rm -r '%s' 2>/dev/null", path_tempdir) < 0) goto end;

	if(s_mkdir(path_tempdir, 0777)) {
		outf("Failed to create directory: %s", path_tempdir);
		goto end;
	}

	outf("Unzipping template document '%s' to tempdir: %s",
	    path_template, path_tempdir);
	e = systemf("unzip -q -d '%s' '%s'", path_tempdir, path_template);
	if(e) {
		outf("Failed to unzip %s into %s",
		    path_template, path_tempdir);
		goto end;
	}

	/* Might be nice to iterate through all items in path_tempdir, but for now
	   we look at just the items that we know extract_docx_content_item() will
	   modify. */

	{
		const char * names[] = { "word/document.xml", "[Content_Types].xml", "word/_rels/document.xml.rels", };
		int names_num = sizeof(names) / sizeof(names[0]);
		for(i = 0; i<names_num; ++i) {
			const char* name = names[i];
			extract_free(&path);
			extract_free(&text);
			extract_free(&text2);
			if(extract_asprintf(&path, "%s/%s", path_tempdir, name) < 0) goto end;
			if(read_all_path(path, &text)) goto end;

			if(extract_docx_content_item(
				    contentss,
				    contentss_num,
				    images,
				    name,
				    text,
				    &text2
				    )) goto end;
			{
				const char* text3 = (text2) ? text2 : text;
				if(write_all(text3, strlen(text3), path)) goto end;
			}
		}
	}

	/* Copy images into <path_tempdir>/media/. */
	extract_free(&path);
	if(extract_asprintf(&path, "%s/word/media", path_tempdir) < 0) goto end;
	if(s_mkdir(path, 0777)) goto end;

	for(i = 0; i<images->images_num; ++i) {
		image_t* image = &images->images[i];
		extract_free(&path);
		if(extract_asprintf(&path, "%s/word/media/%s", path_tempdir, image->name) < 0) goto end;
		if(write_all(image->data, image->data_size, path)) goto end;
	}

	outf("Zipping tempdir to create %s", path_out);
	{
		const char* path_out_leaf = strrchr(path_out, '/');
		if(!path_out_leaf) path_out_leaf = path_out;
		e = systemf("cd '%s' && zip -q -r -D '../%s' .", path_tempdir, path_out_leaf);
		if(e) {
			outf("Zip command failed to convert '%s' directory into output file: %s",
			    path_tempdir, path_out);
			goto end;
		}
	}

	if(!preserve_dir) {
		if(remove_directory(path_tempdir)) goto end;
	}

	e = 0;

end:
	outf("e=%i", e);
	extract_free(&path_tempdir);
	extract_free(&path);
	extract_free(&text);
	extract_free(&text2);
	if(f) fclose(f);

	if(e) {
		outf("Failed to create %s", path_out);
	}
	return e;
}
