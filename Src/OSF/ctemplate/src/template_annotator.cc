// Copyright (c) 2009, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
#include <ctemplate-internal.h>
#pragma hdrstop

// Emits an open annotation string.  'name' must be a string literal.
#define EMIT_OPEN_ANNOTATION(emitter, name, value) (emitter)->Emit("{{#" name "=", 4 + sizeof(name)-1);  \
	(emitter)->Emit(value); (emitter)->Emit("}}", 2);
// Emits a close annotation string.  'name' must be a string literal.
#define EMIT_CLOSE_ANNOTATION(emitter, name) (emitter)->Emit("{{/" name "}}", 5 + sizeof(name)-1);
#define EMIT_MISSING_ANNOTATION(emitter, name, value) (emitter)->Emit("{{" name "=",  3 + sizeof(name)-1);  \
	(emitter)->Emit(value); (emitter)->Emit("}}", 2);

namespace ctemplate {
	using std::string;
	//
	// Implementation note: TextTemplateAnnotator contains no state, and
	// code elsewhere is depending on this.  E.g., a statically allocated
	// instance is used as the default annotator in the implementation of
	// PerExpandData.  If you add state to this class, please revisit
	// the setup of such static instances.
	//
	// This implements precisely the same annotation that was originally
	// built into the template.cc.  Many upstream tools depend on the
	// exact formatting that this implementation happens to produce--
	// so do not consider changes to this lightly.
	void TextTemplateAnnotator::EmitOpenInclude(ExpandEmitter* emitter, const string& value) { EMIT_OPEN_ANNOTATION(emitter, "INC", value); }
	void TextTemplateAnnotator::EmitCloseInclude(ExpandEmitter* emitter) { EMIT_CLOSE_ANNOTATION(emitter, "INC"); }
	void TextTemplateAnnotator::EmitOpenFile(ExpandEmitter* emitter, const string& value) { EMIT_OPEN_ANNOTATION(emitter, "FILE", value); }
	void TextTemplateAnnotator::EmitCloseFile(ExpandEmitter* emitter) { EMIT_CLOSE_ANNOTATION(emitter, "FILE"); }
	void TextTemplateAnnotator::EmitOpenSection(ExpandEmitter* emitter, const string& value) { EMIT_OPEN_ANNOTATION(emitter, "SEC", value); }
	void TextTemplateAnnotator::EmitCloseSection(ExpandEmitter* emitter) { EMIT_CLOSE_ANNOTATION(emitter, "SEC"); }
	void TextTemplateAnnotator::EmitOpenVariable(ExpandEmitter* emitter, const string& value) { EMIT_OPEN_ANNOTATION(emitter, "VAR", value); }
	void TextTemplateAnnotator::EmitCloseVariable(ExpandEmitter* emitter) { EMIT_CLOSE_ANNOTATION(emitter, "VAR"); }
	void TextTemplateAnnotator::EmitFileIsMissing(ExpandEmitter* emitter, const string& value) { EMIT_MISSING_ANNOTATION(emitter, "MISSING_FILE", value); }
}
