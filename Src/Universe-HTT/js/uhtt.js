// Universe-HTT JS core
// uhtt.js, 
// (c) Michael Kazakov, A.Sobolev 2011, 2012, 2013, 2014, 2015, 2016, 2020
// D:\Universe-HTT\liferay-portal-6.1.1\tomcat-7.0.27\webapps\rsrc\js\uhtt.js
//
// Common
//
function isNull(arg0) { return ((arg0 == null) || (arg0 == undefined)); }
function isInteger(arg0) { return !isNull(arg0) && ((arg0 % 1) == 0); }
function isBoolean(arg0) { return !isNull(arg0) && ((typeof arg0) == "boolean"); }
function isString(arg0) { return !isNull(arg0) && ((typeof arg0) == "string"); }
function isArray(arg0) { return !isNull(arg0) && (arg0.constructor == Array); }
function isFunction(arg0) { return !isNull(arg0) && ((typeof arg0) == "function"); }

function isEmpty(arg0) 
{
	var r = true;
	if(!isNull(arg0)) {
		r = false;
		if(isString(arg0))
			r = (arg0.length > 0) ? false : true;
		else if(isArray(arg0))
			r = (arg0.length > 0) ? false : true;
		else if(arg0 instanceof SList)
			r = arg0.isEmpty();
	}
	return r;
}

function ToInt(arg0) {
	var id = 0;
	if(!isEmpty(arg0) && !isNaN(arg0))
		id = parseInt(arg0);
	return id;
}
//
// Функция сравнения объектов
//
function objectCompare(obj1, obj2) {
	// check parameters
	for(var p in obj1) {
		if(typeof(obj2[p]) == "undefined")
			return false;
	}
	for(var p in obj2) {
		if(typeof(obj1[p]) == "undefined")
			return false;
	}
	/* check type and value */
	for(var p in obj1) {
		var v1 = obj1[p];
		var v2 = obj2[p];
		// check types
		if(typeof(v1) != typeof(v2))
			return false;
		// check values
		switch(typeof(v1)) {
			case "object":
				if(!objectCompare(v1, v2))   // @recursion
					return false;
				break;
			case "function":
				if(v1.toString() != v2.toString())
					return false;
				break;
			default:
				if(v1 != v2)
					return false;
				break;
		}
	}
	return true;
};
//
// Extend object
//
function _extends(base, obj) 
{
	return jQuery.extend(obj, base);
};
//
// Clone objects
//
function clone(obj) 
{
	function _Clone() 
	{
	}
    _Clone.prototype = obj;
    return new _Clone();
}
//
// Default ajax error handler
//
function AJAX_ERR_HANDLER(request, status, error) 
{
	var msg = decodeURIComponent(request.getResponseHeader("X-UHTT-Error"));
	if(!isEmpty(msg))
		UHTT.Messenger.show(msg, "/rsrc/images/error_48.png");
}
//
// 
//
//
var proxy = function(fn, context) {
	return (
		function() { 
			return fn.apply(context, arguments); 
		}
	);
};
//
// jQuery ext
//
jQuery.fn.exists = function() 
{ 
	return (this.length > 0); 
};

jQuery.fn.hasBind = function(event) 
{
	var events = this.data("events");
	var result = false;
	if(!isEmpty(events)) {
		$.each(events, 
			function(e, h) {
				if(event.trim() == e.trim()) {
					result = true;
					return false;
				}
			}
		);
	}
	return result;
};

jQuery.fn.Evt = function(evt, handler, reset) 
{
	if(this.hasBind(evt)) {
		if(reset)
			this.unbind(evt);
		else
			return this;
	}
	this.bind(evt, handler);
	return this;
};

jQuery.fn.showErrorMessage = function(msg) 
{
	var $blk = $(this);
	var counter = $blk.attr("_counter");
	if(counter == undefined)
		$blk.attr("_counter", 1);
	else
		$blk.attr("_counter", ++counter);
	$blk.html(msg);
	$blk.fadeIn(10, 
		function() {
			setTimeout(
				function() 
				{
					counter = $blk.attr("_counter");
					$blk.attr("_counter", --counter);
					if(counter <= 0) {
						$blk.fadeOut(300, 
							function() 
							{
								$blk.html("");
							}
						);
					}
				}, 
			5000);
		}
	);
	return this;
};
jQuery.fn.setParentWidth = function(obj) 
{
	this.width(this.parent().width());
	this.height(this.width());
};
jQuery.fn.initTip = function(_cls) 
{
	var $blk = $(this);
	var cls = !isEmpty(_cls) ? _cls : "uhtt-tip";
	var ref = { "is_hovered":false, "tip":$blk.children("." + cls) };
	$blk.Evt("mouseenter", 
		function(evt) 
		{
			ref.is_hovered = true;
			setTimeout(
				function() 
				{
					if(ref.is_hovered)
						ref.tip.slideDown(200);
				}, 500);
		}, true).Evt("mouseleave", 
			function(evt) 
			{
				ref.is_hovered = false;
				ref.tip.slideUp(200);
			}, true);
	return this;
};
//
// Types
//
var SPair = function(key, value) {
	this.Key = key;
	this.Value = value;
};

var SList = function(_array) {
	// private
	var array = null;
	// public
	this.getArray = function() 
	{
		return array.slice(0);
	};
	this.setArray = function(arg0) 
	{
		var ok = true;
		if(!isEmpty(arg0)) {
			if(isArray(arg0))
				array = arg0.slice(0);
			else
				ok = false;
		}
		else
			array = [];
		return ok;
	};
	this.getCount = function() 
	{
		return array.length;
	};
	this.isEmpty = function() 
	{
		return (array.length == 0) ? true : false;
	};
	this.flush = function() 
	{
		array = [];
	};
	this.add = function(arg0) 
	{
		if(!isEmpty(arg0))
			array.push(arg0);
	};
	this.addUnique = function(arg0) 
	{
		if(!isEmpty(arg0)) {
			var skip = false;
			for(var i = 0, n = array.length; (i < n) && !skip; i++) {
				var item = this.at(i);
				var typ = typeof(item);
				if(typ == typeof(arg0)) {
					switch(typ) {
						case "object":
							skip = (objectCompare(item, arg0));
							break;
						case "function":
							skip = (item.toString() == arg0.toString());
							break;
						default:
							skip = (item == arg0);
							break;
					}
				}
			}
			if(!skip)
				this.add(arg0);
		}
	};
	this.remove = function(idx) 
	{
		if(idx > -1 && idx < array.length)
			array.splice(idx, 1);
	};
	this.at = function(idx) 
	{
		var obj = null;
		if(idx > -1 && idx < array.length)
			obj = array[idx];
		return obj;
	};
	this.getIdxByProperty = function(prop, v) 
	{
		var idx = -1;
		if(!isEmpty(prop) && !isEmpty(v)) {
			var item = null;
			var n_items = array.length;
			for(var i = 0; i < n_items; i++) {
				item = array[i];
				if(!isEmpty(item[prop])) {
					if(item[prop] == v) {
						idx = i;
						break;
					}
				}
			}
		}
		return idx;
	};
	//
	// Находит индекс элемента по паре свойств {prop1 == v1}, {prop2 == v2}
	//	
	this.getIdxByProperty2 = function(prop1, v1, prop2, v2) {
		var idx = -1;
		if(!isEmpty(prop1) && !isEmpty(v1) && !isEmpty(prop2) && !isEmpty(v2)) {
			var item = null;
			var n_items = array.length;
			for(var i = 0; i < n_items; i++) {
				item = array[i];
				if(!isEmpty(item[prop1]) && !isEmpty(item[prop2])) {
					if(item[prop1] == v1 && item[prop2] == v2) {
						idx = i;
						break;
					}
				}
			}
		}
		return idx;
	};	
	this.getObjByProperty = function(prop, v) {
		var obj = null;
		var idx = this.getIdxByProperty(prop, v);
		if(idx > -1)
			obj = array[idx];
		return obj;
	};
	//
	// Находит элемент по паре свойств {prop1 == v1}, {prop2 == v2}
	//
	this.getObjByProperty2 = function(prop1, v1, prop2, v2) {
		var obj = null;
		var idx = this.getIdxByProperty2(prop1, v1, prop2, v2);
		if(idx > -1)
			obj = array[idx];
		return obj;
	};	
	this.getValue = function(key) {
		var value = null;
		var item = null;
		var n = this.getCount();
		for(var i = 0; i < n; i++) {
			item = this.at(i);
			if(!isEmpty(item)) {
				var v = item[key];
				if(!isEmpty(v)) {
					value = v;
					break;
				}
			}
		}
		return value;
	};
	// init
	{
		if(!this.setArray(_array))
			throw new Error("Error: SList init failed (argument '_array' isn`t array)");
	}
};

var SDialog = function(id, source, options, type) 
{
	// private
	var $inst = null;
	// public
	this.ID = id;
	this.closeHandler = null;
	this.close = function() 
	{
		if($inst != null)
			$inst.dialog("close");
	};
	this.setCloseHandler = function(handler, args) 
	{
		if(!isEmpty(handler) && typeof(handler) == "function")
			this.closeHandler = { fn: handler, args: args };
		else
			this.closeHandler = null;
	};
	this.setOption = function(op, value) 
	{
		if($inst != null)
			$inst.dialog("option", op, value);
	};
	this.getOption = function(op) 
	{
		var value = null;
		if($inst != null)
			value = $inst.dialog("option", op);
		return value;
	};
	// init 
	if(isEmpty(this.ID))
		this.ID = UHTT.Dialog.generateID();
	else {
		var _inst = UHTT.Dialog.List.getObjByProperty("ID", this.ID);
		if(_inst != null)
			_inst.close();
	}
	if(!isEmpty(this.ID)) {
		UHTT.Dialog.List.add(this);
		UHTT.Dialog.Container().append('<div id="' + this.ID + '" class="uhtt-dialog"></div>');
		$inst = $("#" + this.ID);
		$inst.dialog(
			{
				autoOpen: false,
				width: "auto",
				height: "auto",
				resizable: false,
				position: [20 * UHTT.Dialog.List.getCount(), 20 * UHTT.Dialog.List.getCount()],
				close: function(ev, ui) {
					var _id = $(this).attr("id");
					var idx = UHTT.Dialog.List.getIdxByProperty("ID", _id);
					if(idx > -1) {
						var _inst = UHTT.Dialog.List.at(idx);
						if(!isEmpty(_inst.closeHandler))
							_inst.closeHandler.fn(_inst.closeHandler.args);
						UHTT.Dialog.List.remove(idx);
					}
					$(this).remove();
				}
			}
		);
		var value = null;
		if(!isEmpty(options)) {
			for(var op in options) {
				value = options[op];
				if(!isEmpty(value))
					this.setOption(op, value);
			}
		}
		if(isNull(type))
			type = UHTT.Dialog.TYPE.LOADABLE;
		var data = null;
		switch(type) {
			case UHTT.Dialog.TYPE.LOADABLE:
				data = UHTT.requestData(null, source);
				break;
			case UHTT.Dialog.TYPE.INLINE:
				data = source;
				break;
			default:
				throw new Error("Error: Unknown dialog type");
				break;
		}
		if(!isEmpty(data)) {
			$inst.html(data);
			$inst.dialog("open");
		}
	};
};

var SPrint = function(id, source, options, type)
{
	// private
	var $inst = null;
	// public
	this.ID = id;
	this.closeHandler = null;
	this.close = function() 
	{
		if($inst != null)
			$inst.dialog("close");
	};
	this.setCloseHandler = function(handler, args) 
	{
		if(!isEmpty(handler) && typeof(handler) == "function")
			this.closeHandler = { fn: handler, args: args };
		else
			this.closeHandler = null;
	};
	this.setOption = function(op, value) 
	{
		if($inst != null)
			$inst.dialog("option", op, value);
	};
	this.getOption = function(op) 
	{
		var value = null;
		if($inst != null)
			value = $inst.dialog("option", op);
		return value;
	};
	// init 
	if(isEmpty(this.ID))
		this.ID = UHTT.Dialog.generateID();
	else {
		var _inst = UHTT.Dialog.List.getObjByProperty("ID", this.ID);
		if(_inst != null)
			_inst.close();
	}
	if(!isEmpty(this.ID)) {
		UHTT.Dialog.List.add(this);
		UHTT.Dialog.Container().append('<div id="' + this.ID + '" class="uhtt-dialog"></div>');
		$inst = $("#" + this.ID);
		$inst.dialog(
			{
				autoOpen: false,
				width: "auto",
				height: "auto",
				resizable: false,
				position: [20 * UHTT.Dialog.List.getCount(), 20 * UHTT.Dialog.List.getCount()],
				close: function(ev, ui) {
					var _id = $(this).attr("id");
					var idx = UHTT.Dialog.List.getIdxByProperty("ID", _id);
					if(idx > -1) {
						var _inst = UHTT.Dialog.List.at(idx);
						if(!isEmpty(_inst.closeHandler))
							_inst.closeHandler.fn(_inst.closeHandler.args);
						UHTT.Dialog.List.remove(idx);
					}
					$(this).remove();
				}
			}
		);
		var value = null;
		if(!isEmpty(options)) {
			for(var op in options) {
				value = options[op];
				if(!isEmpty(value))
					this.setOption(op, value);
			}
		}
		if(isNull(type))
			type = UHTT.Dialog.TYPE.LOADABLE;	
		var prt_content = null;
		switch(type) {
			case UHTT.Dialog.TYPE.LOADABLE:
				prt_content = UHTT.requestData(null, source);
				break;
			case UHTT.Dialog.TYPE.INLINE:
				prt_content = source;
				break;
			default:
				throw new Error("Error: Unknown dialog type on printing");
				break;
		}	
		if(!isEmpty(prt_content)) {
			var prt_css = ''; // '<link rel="stylesheet" href="/templates/css/template.css" type="text/css" />';
			var win_print = window.open('','','left=50,top=50,width=800,height=640,toolbar=0,scrollbars=1,status=0');
			//win_print.document.write('<div id="print" class="contentpane">');
			//win_print.document.write(prt_css);
			win_print.document.write(prt_content);
			//win_print.document.write('</div>');
			win_print.document.close();
			win_print.focus();
			win_print.print();
			win_print.close();
			//prt_content.innerHTML=strOldOne;
		}
		else {
			throw new Error("Error: empty print content");
		}
	}
};
//
// Editor prototype
//
var BaseEditor = {
	Args : new SList(),
	EventHandlers : new SList(),
	IsEditing : false,
	Object : null,
	addEventHandler : function(handler) 
	{
		if(!isEmpty(handler) && typeof(handler) == "function")
			this.EventHandlers.addUnique({ fn: handler });
	},
	processEvent : function(type, ctx) 
	{
		if(!this.EventHandlers.isEmpty()) {
			var n = this.EventHandlers.getCount();
			for(var i = 0; i < n; i++) {
				var handler = this.EventHandlers.at(i);
				if(!isEmpty(handler))
					handler.fn(type, ctx);
			}
		}
	},
	getArg : function(arg) 
	{
		return this.Args.getValue(arg);
	}
};
//
// String extention
//
String.prototype.escapeHtml = function() 
{
	var text = this.toString();
	var chars = [ "&", "<", ">", '"' ];
	var entities = [ "&amp;", "&lt;", "&gt;", "&quot;" ];
	for(var i = 0; i < chars.length; i++) {
		var re = new RegExp(chars[i], "gi");
		if(re.test(text))
			text = text.replace(re, entities[i]);
	}
	return text;
};

String.prototype.unescapeHtml = function() 
{
	var text = this.toString();
	var chars = [ "&", "<", ">", '"' ];
	var entities = [ "&amp;", "&lt;", "&gt;", "&quot;" ];
	for(var i = 0; i < entities.length; i++) {
		var re = new RegExp(entities[i], "gi");
		if(re.test(text))
			text = text.replace(re, chars[i]);
	}
	return text;
};

String.prototype.screening = function(ch, screen) 
{
	var text = this.toString();
	var re = new RegExp(ch, "gi");
	if(re.test(text))
		text = text.replace(re, screen + ch);
	return text;
};

String.prototype.toUnicode = function() 
{
	var text = this.toString();
	var unicodeString = '';
	for(var i = 0; i < text.length; i++) {
		var theUnicode = text.charCodeAt(i).toString(16).toUpperCase();
		while(theUnicode.length < 4) {
			theUnicode = '0' + theUnicode;
		}
		theUnicode = '\\u' + theUnicode;
		unicodeString += theUnicode;
	}
	return unicodeString;
};
//
// Date extention
//
Date.prototype.format = function (mask, utc) 
{
	return dateFormat(this, mask, utc);
};
//
// XTree
//
;(function($) {
	var _CLASS = {
		item: "xtree-item",
		hitarea: "xtree-hitarea",
		hitareaOpened: "xtree-hitarea-opened",
		hitareaClosed: "xtree-hitarea-closed",
		itemContainer: "xtree-item-container",
		itemImage: "xtree-item-img",
		itemText: "xtree-item-text",
		itemClicked: "xtree-item-clicked"
	};
	var _DEFAULTS = {
		addRoot: false,
		rootName: "root",
		collapsed: true,
		handler: function() { return false; }
	};
	var insertTreeItem = function(root_id, list, item) {
		var id, parent_id, txt;
		var cont_id;
		id = item.ID;
		parent_id = item.ParentID;
		txt = item.Name;
		var line = 
			'<li id="' + root_id + "_li_" + id + '">' +
				'<div itemID="' + id + '" class="' + root_id + '_item xtree-item-container">' +
					'<img class="xtree-item-img" src="/rsrc/images/folder.png" />' +
					'<div class="xtree-item-text-blk"><span class="xtree-item-text">' + txt + '</span></div>' +
				'</div>' +
			'</li>';
		if(parent_id == 0)
			$("#" + root_id).append(line);
		else {
			if($("#" + root_id + "_li_" + parent_id).length == 0) {
				var parent_idx = list.getIdxByProperty("Id", parent_id);
				var parent = list.at(parent_idx);
				if(parent != null) {
					list.remove(parent_idx);
					insertTreeItem(root_id, list, parent);
				}
				else
					return;
			}
			cont_id = root_id + "_ul_" + parent_id;
			if($("#" + cont_id).length == 0)
				$("#" + root_id + "_li_" + parent_id).append('<ul id="' + cont_id + '"></ul>');
			$("#" + cont_id).append(line);
		}
	};
	var constructTree = function(config, list) {
		var tree = $("#" + config.tree_id);
		var root_id = config.tree_id;
		tree.html("");
		if(config.addRoot) {
			tree.append(
				'<li id="' + config.tree_id + '_li_0">' +
					'<div itemID="0" class="' + config.tree_id + '_item xtree-item-container">' +
						'<img class="xtree-item-img" src="/standard-theme/images/uhtt/folder.png" />' +
						'<div class="xtree-item-text-blk"><span class="xtree-item-text">' + config.rootName + '</span></div>' + 
					'</div>' +
					'<ul id="' + config.tree_id + '_ul_0"></ul>' +
				'</li>'
			);
			root_id = config.tree_id + "_ul_0";
		}
		var item = null;
		for(var i = 0; i < list.getCount(); i++) {
			item = list.at(i);
			if(item.ID > 0)
				insertTreeItem(root_id, list, item);
		}
	};
	$.extend($.fn, {
		xtree : function(ary, settings) {
			var tree = $(this);
			var config = {};
			$.extend(config, _DEFAULTS, settings);
			config.tree_id = tree.attr("id");
			if(!isEmpty(config.tree_id) && (ary instanceof SList)) {
				constructTree(config, ary);
				this.find("li").addClass(_CLASS.item);
				this.find("li:has(ul)").prepend('<div class="' + _CLASS.hitarea + ' ' + (config.collapsed ? _CLASS.hitareaClosed : _CLASS.hitareaOpened) + '" />');
				if(config.collapsed)
					this.find("." + _CLASS.item).children("ul").hide();
				tree
					.undelegate("." + _CLASS.hitarea, "click")
					.undelegate("." + _CLASS.itemContainer, "mousedown")
					.undelegate("." + _CLASS.itemContainer, "dblclick");
				tree
					.delegate("." + _CLASS.hitarea, "click", 
						function() {
							var _hitarea = $(this);
							if(_hitarea.hasClass(_CLASS.hitareaOpened)) {
								_hitarea.removeClass(_CLASS.hitareaOpened).addClass(_CLASS.hitareaClosed);
								_hitarea.parent().children("ul").hide(50);
							}
							else {
								_hitarea.removeClass(_CLASS.hitareaClosed).addClass(_CLASS.hitareaOpened);
								_hitarea.parent().children("ul").show();
							}
						}
					)
					.delegate("." + _CLASS.itemContainer, "mousedown",  
						function() {
							tree.find("span." + _CLASS.itemClicked).removeClass(_CLASS.itemClicked);
							$(this).find("span").addClass(_CLASS.itemClicked);
						}
					)
					.delegate("." + _CLASS.itemContainer, "dblclick",  config.handler);
			}
		},
		/*
		 * For velocity generated tree
		 */
		_xtree : function(settings) {
			var tree = $(this);
			var config = {};
			$.extend(config, _DEFAULTS, settings);
			if(config.collapsed)
				this.find("." + _CLASS.item).children("ul").hide();
			tree
				.undelegate("." + _CLASS.hitarea, "click")
				.undelegate("." + _CLASS.itemContainer, "mousedown")
				.undelegate("." + _CLASS.itemContainer, "dblclick");
			tree
				.delegate("." + _CLASS.hitarea, "click", 
					function() {
						var _hitarea = $(this);
						if(_hitarea.hasClass(_CLASS.hitareaOpened)) {
							_hitarea.removeClass(_CLASS.hitareaOpened).addClass(_CLASS.hitareaClosed);
							_hitarea.parent().children("ul").hide(50);
						}
						else {
							_hitarea.removeClass(_CLASS.hitareaClosed).addClass(_CLASS.hitareaOpened);
							_hitarea.parent().children("ul").show();
						}
					}
				)
				.delegate("." + _CLASS.itemContainer, "mousedown",  
					function() {
						tree.find("span." + _CLASS.itemClicked).removeClass(_CLASS.itemClicked);
						$(this).find("span").addClass(_CLASS.itemClicked);
					}
				)
				.delegate("." + _CLASS.itemContainer, "dblclick",  config.handler);
		}
	});
})(jQuery);
//
// SUrl
//
var SUrl = function(arg0) 
{
	this.init = function() 
	{
		this.Scheme = "";
		this.Host = "";
		this.Port = 0;
		this.Path = "";
		this.Parameters = new SList();
		this.Anchor = "";
	};
	this.parse = function(arg0) 
	{
		this.init();
		var pattern = "^(([^:/\\?#]+):)?(//(([^:/\\?#]*)(?::([^/\\?#]*))?))?([^\\?#]*)(\\?([^#]*))?(#(.*))?$";
		var parts = new RegExp(pattern).exec(arg0);
	    this.Scheme = parts[2];
	    this.Host = parts[5];
	    this.Port = parts[6];
	    this.Path = parts[7];
	    {
	    	// parse parameters
			var pstr = parts[9];
			if(!isEmpty(pstr)) {
				var chunks = pstr.split('&');
				if(!isEmpty(chunks)) {
					for(var i = 0, n = chunks.length; i < n; i++) {
						var chunk = chunks[i];
						if(!isEmpty(chunk)) {
							var pair = chunk.split('=');
							if(!isEmpty(pair) && (pair.length == 2) && !isEmpty(pair[0]) && !isEmpty(pair[1])) {
								this.Parameters.add(new SPair(pair[0], pair[1]));
							}
							else {
								console.warn("URL parsing: invalid syntax");
							}
						}
					}
				}
			}
	    }
	    this.Anchor = parts[11];
	};
	this.toString = function() 
	{
		var _str = "";
		if(!isEmpty(this.Host)) {
			if(!isEmpty(this.Scheme))
				_str = this.Scheme + "://";
			_str += this.Host;
			if(this.Port > 0)
				_str += ":" + this.Port;
			_str += "/";
		}
		if(!isEmpty(this.Path)) {
			var path = this.Path;
			var startWithSlash = (path.charAt(0) == '/');
			if(!isEmpty(_str)) {
				if(startWithSlash)
					path = path.slice(1);
			}
			else {
				if(!startWithSlash)
					path = "/" + path;
			}
			_str += path;
		}
		if(!this.Parameters.isEmpty()) {
			_str += "?";
			var first = true;
			for(var i = 0, n = this.Parameters.getCount(); i < n; i++) {
				var pair = this.Parameters.at(i);
				if(pair != null) {
					if(!isEmpty(pair.Key) && !isEmpty(pair.Value))
						_str += (first ? "" : "&") + pair.Key + "=" + encodeURIComponent(pair.Value);
				}
				first = false;
			}
		}
		if(!isEmpty(this.Anchor))
			_str += "#" + this.Anchor;
		return _str;
	};
	//
	this.init();
	if(!isEmpty(arg0))
		this.parse(arg0);
};
//
// UHTT Core
//
var UHTT = {
	//
	// Utilities
	//
	Util : {
		URI : {
			buildQuery : function(query, param, value) {
				if(isEmpty(query)) {
					query = "?" + param + "=" + encodeURIComponent(value);
				}
				else {
					query += "&" + param + "=" + encodeURIComponent(value);
				}
				return query;
			}
		},
		Cookie : {
			set : function(name, value, expires, path, domain, secure) 
			{
				document.cookie = name + "=" + escape(value) +
					((expires) ? "; expires=" + expires : "") + ((path) ? "; path=" + path : "") + ((domain) ? "; domain=" + domain : "") + ((secure) ? "; secure" : "");
			},
			get : function(name) 
			{
				var cookie = " " + document.cookie;
				var search = " " + name + "=";
				var setStr = null;
				var offset = 0;
				var end = 0;
				if(cookie.length > 0) {
					offset = cookie.indexOf(search);
					if(offset != -1) {
						offset += search.length;
						end = cookie.indexOf(";", offset);
						if(end == -1)
							end = cookie.length;
						setStr = unescape(cookie.substring(offset, end));
					}
				}
				return(setStr);
			},
			remove : function(name) 
			{
				if(this.get(name))
					this.set(name, "", "Thu, 01-Jan-1970 00:00:01 GMT", "/");
			},
			expireByHours : function(hours) 
			{
				var dt = new Date();
				dt.setHours(dt.getHours() + hours);
				return dt;
			},
			expireByMinutes : function(mins) 
			{
				var dt = new Date();
				dt.setMinutes(dt.getMinutes() + mins);
				return dt;
			},
			neverExpire : function() 
			{
				var dt = new Date();
				dt.setHours(dt.getHours() + (24 * 365 * 10));   // 10 years
				return dt;
			}
		},
		JSON : {
			toString : function(obj) 
			{
				var t = typeof(obj);
				if(t != "object" || obj == null) {
					// simple data type
					if(t == "string")
						obj = '"' + obj.toUnicode() + '"';
					return String(obj);
				}
				else {
					// recurse array or object
					var v, json = [], arr = (obj && (obj.constructor == Array));
					for(var n in obj) {
						v = obj[n];
						t = typeof(v);
						if(t == "string")
							v = '"' + v.toUnicode() + '"';
						else if(t == "object" && v !== null)
							v = this.toString(v);
						json.push((arr ? "" : '"' + n + '":') + String(v));
					}
					return (arr ? "[" : "{") + String(json) + (arr ? "]" : "}");
				}
			},
			toStringSkipItem : function(obj, skipItemName) 
			{
				var t = typeof(obj);
				if(t != "object" || obj == null) {
					// simple data type
					if(t == "string")
						obj = '"' + obj.toUnicode() + '"';
					return String(obj);
				}
				else {
					// recurse array or object
					var v, json = [], arr = (obj && (obj.constructor == Array));
					for(var n in obj) {
						if(n != skipItemName) {
							v = obj[n];
							t = typeof(v);
							if(t == "string")
								v = '"' + v.toUnicode() + '"';
							else if(t == "object" && v !== null)
								v = this.toStringSkipItem(v, skipItemName); // @recursion
							json.push((arr ? "" : '"' + n + '":') + String(v));
						}
					}
					return (arr ? "[" : "{") + String(json) + (arr ? "]" : "}");
				}
			},			
			parse : function(json) 
			{
				var obj = null;
				if(!isEmpty(json))
					obj = eval("(" + json + ")");
				return obj;
			},
			getObjects : function(obj, key, val) 
			{
				var objects = [];
				for(var i in obj) {
					if(obj.hasOwnProperty(i)) {
						if(typeof obj[i] == "object")
							objects = objects.concat(this.getObjects(obj[i], key, val));
						else if(i == key && obj[key] == val)
							objects.push(obj);
					}
				}
				return objects;
			}
		},
		Base64 : {
			_symb : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=",
			encode : function (input) 
			{
				var output = "";
				var chr1, chr2, chr3, enc1, enc2, enc3, enc4;
				var i = 0;
				input = this._utf8_encode(input);
				while(i < input.length) {
					chr1 = input.charCodeAt(i++);
					chr2 = input.charCodeAt(i++);
					chr3 = input.charCodeAt(i++);
					enc1 = chr1 >> 2;
					enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
					enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
					enc4 = chr3 & 63;
					if(isNaN(chr2)) {
						enc3 = enc4 = 64;
					} 
					else if(isNaN(chr3)) {
						enc4 = 64;
					}
					output = output +
					this._symb.charAt(enc1) + this._symb.charAt(enc2) +
					this._symb.charAt(enc3) + this._symb.charAt(enc4);
				}
				return output;
			},
			decode : function (input) 
			{
				var output = "";
				var chr1, chr2, chr3;
				var enc1, enc2, enc3, enc4;
				var i = 0;
				input = input.replace(/[^A-Za-z0-9\+\/\=]/g, "");
				while (i < input.length) {
					enc1 = this._symb.indexOf(input.charAt(i++));
					enc2 = this._symb.indexOf(input.charAt(i++));
					enc3 = this._symb.indexOf(input.charAt(i++));
					enc4 = this._symb.indexOf(input.charAt(i++));
					chr1 = (enc1 << 2) | (enc2 >> 4);
					chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
					chr3 = ((enc3 & 3) << 6) | enc4;
					output = output + String.fromCharCode(chr1);
					if (enc3 != 64) {
						output = output + String.fromCharCode(chr2);
					}
					if (enc4 != 64) {
						output = output + String.fromCharCode(chr3);
					}
				}
				output = this._utf8_decode(output);
				return output;
			},
			_utf8_encode : function(string) 
			{
				string = string.replace(/\r\n/g,"\n");
				var utftext = "";
				for (var n = 0; n < string.length; n++) {
					var c = string.charCodeAt(n);
					if (c < 128) {
						utftext += String.fromCharCode(c);
					}
					else if((c > 127) && (c < 2048)) {
						utftext += String.fromCharCode((c >> 6) | 192);
						utftext += String.fromCharCode((c & 63) | 128);
					}
					else {
						utftext += String.fromCharCode((c >> 12) | 224);
						utftext += String.fromCharCode(((c >> 6) & 63) | 128);
						utftext += String.fromCharCode((c & 63) | 128);
					}
				}
				return utftext;
			},
			_utf8_decode : function(utftext) 
			{
				var string = "";
				var i = 0;
				var c = c1 = c2 = 0;
				while(i < utftext.length) {
					c = utftext.charCodeAt(i);
					if (c < 128) {
						string += String.fromCharCode(c);
						i++;
					}
					else if((c > 191) && (c < 224)) {
						c2 = utftext.charCodeAt(i+1);
						string += String.fromCharCode(((c & 31) << 6) | (c2 & 63));
						i += 2;
					}
					else {
						c2 = utftext.charCodeAt(i+1);
						c3 = utftext.charCodeAt(i+2);
						string += String.fromCharCode(((c & 15) << 12) | ((c2 & 63) << 6) | (c3 & 63));
						i += 3;
					}
				}
				return string;
			}
		}
	},
	Auth : {
		login : function(name, password, rememberme, success_handler, error_handler, done_handler)
		{
			var url = "/dispatcher/auth?name=" + encodeURIComponent(name) + "&password=" + encodeURIComponent(password);
			if(isInteger(rememberme))
				url = url + "&rememberme=" + encodeURIComponent(rememberme);
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: success_handler,
					error: error_handler
				}
			).done(done_handler);		
		},
		logout : function(success_handler, error_handler, done_handler)
		{
			var url = "/dispatcher/auth?name=" + encodeURIComponent("$logout$") + "&password=" + encodeURIComponent("");
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: success_handler,
					error: error_handler
				}
			).done(done_handler);		
		}
	},
	//
	// Блокировка экрана
	//
	BlockUI : {
		setup : function(text, img_url) 
		{
			var msg_blk = '<div class="uhtt-block-ui-layout" id="uhtt_block_ui_layout"></div> \
				<div class="uhtt-block-ui-message" id="uhtt_block_ui_message"> \
				<table><tbody><tr><td><img src="' + img_url + '"/></td><td style="padding:0px 10px"><h2>' + text +
				'</h2></td><tr></tbody></table></div>';
			$(".uhtt-block-ui-message").remove();
			$(".uhtt-block-ui-layout").remove();
			$("body").append(msg_blk);
		},
		block : function() 
		{
			$("#uhtt_block_ui_layout").show();
			$("#uhtt_block_ui_message").show();
		},
		unblock : function() 
		{
			$("#uhtt_block_ui_message").fadeOut(200, 
				function() {
					$("#uhtt_block_ui_layout").hide();
				}
			);
		}
	},
	// Всплывающее сообщение
	Messenger : {
		show : function(msg, img_url, delay) 
		{
			var content = isEmpty(img_url) ? msg : 
				'<table><tbody><tr><td><img src="' + img_url + '"/></td><td style="padding:0px 10px">' + msg + '</td><tr></tbody></table>';
			var $pop_msg = $("#uhtt_popup_message");
			if(isEmpty(delay))
				delay = 5000;
			if(!$pop_msg.exists()) {
				$("body").append('<div class="uhtt-popup-message" id="uhtt_popup_message" _counter="0"></div>');
				$pop_msg = $("#uhtt_popup_message");
			}
			$pop_msg.hide();
			$pop_msg.html(content);
			var counter = $pop_msg.attr("_counter");
			$pop_msg.attr("_counter", ++counter);
			$pop_msg.fadeIn(300, 
				function() 
				{
					setTimeout(
						function() 
						{
							counter = $pop_msg.attr("_counter");
							$pop_msg.attr("_counter", (counter > 0) ? --counter : 0);
							if(counter == 0)
								$pop_msg.fadeOut(300);
						}, delay);
				}
			);
		}
	},
	// События
	Event : {
		setGlobal : function(event, handler) 
		{
			if(!$(document).hasBind(event))
				$(document).bind(event, handler);
		},
		resetGlobal : function(event) 
		{
			if($(document).hasBind(event))
				$(document).unbind(event);
		},
		fire : function(event, data) 
		{
			if(!isEmpty(event))
				$(document).trigger(event, data);
		}
	},
	// Drop-down объекты
	DDO : {
		Initialize : false,
		List : new SList(),
		initListener : function() {
			$("body").Evt("mouseup", function(event) {
				var ref = null;
				var n = UHTT.DDO.List.getCount();
				for(var i = 0; i < n;) {
					ref = UHTT.DDO.List.at(i);
					if(ref.$obj.exists()) {
						if(ref.is_hovered == false)
							ref.$obj.hide();
						i++;
					}
					else
						UHTT.DDO.List.remove(i);
				}
			}, true);
			this.Initialize = true;
		},
		initObject : function(obj) 
		{
			if(isEmpty(obj) || !(obj instanceof $))
				return false;
			if(!this.Initialize)
				this.initListener();
			{
				var id = obj.attr("id");
				if(isEmpty(id))
					return false;
				var n = this.List.getCount();
				for(var i = 0; i < n; i++) {
					var ref_i = this.List.at(i);
					if(ref_i.$obj.attr("id") == id) {
						this.List.remove(i);
						break;
					}
				}
			}
			var ref = { "is_hovered":false, "$obj":obj };
			this.List.add(ref);
			ref.$obj.Evt("mouseenter", 
				function() 
				{ 
					ref.is_hovered = true; 
				}, true)
				.Evt("mouseleave", 
					function() 
					{ 
						ref.is_hovered = false; 
					}, true);
			return ref;
		}
	},
	DropDownBlock : {
		Initialize : false,
		Blk : null,
		//
		init : function() {
			var _this = this;
			// get jQuery object wrapper
			_this.Blk = $("#uui_dd_blk");
			if(!_this.Blk.exists()) {
				// insert blk in to DOM
				$("body").append('<div id="uui_dd_blk"></div>');
				_this.Blk = $("#uui_dd_blk");
			}
			// set event handler
			$(document).Evt("mouseup", function(e) {
				//if(!_this.Blk.has(e.target).exists())  {
					_this.Blk.undelegate();  // reset all event handlers
					_this.Blk.hide();        // hide it
				//}
			});
			_this.Blk.Evt("mouseup", function() {
				//return false;
			});
			// init done
			_this.Initialize = true;
		},
		show : function(anchor, content, handlers) {
			if(!this.Initialize)
				this.init();
			//
			this.Blk.html(content);
			this.Blk.fadeIn(250);
			// 
			var anchor_x = anchor.offset().left;
			var anchor_y = anchor.offset().top;
			var anchor_h = anchor.outerHeight(true);
			var anchor_w = anchor.outerWidth(true);
			// position setup
			var x = anchor_x;
			var y = anchor_y + anchor_h + 1;
			//
			var window_height = $(window).height();
			var window_width = $(window).width();
			var blk_height = this.Blk.outerHeight(true);
			var blk_width = this.Blk.outerWidth(true);
			//
			// check horizontal overflow
			if(x + blk_width > window_width) {
				x = anchor_x - blk_width + anchor_w;
			}
			//
			this.Blk.offset({ left: x, top: y });
			//
			// check blk height & window height
			if(blk_height > window_height) {
				y = $(document).scrollTop();
				this.Blk.offset({ left: x, top: y });
			}
			else {
				// check bottom overflow
				var window_bottom = $(document).scrollTop() + window_height;
				var blk_bottom = this.Blk.offset().top + this.Blk.height();
				if(blk_bottom > window_bottom) {
					y = anchor_y - blk_height - 1;
					this.Blk.offset({ left: x, top: y });
				}
				// check top overflow
				var window_top = $(document).scrollTop();
				var blk_top = this.Blk.offset().top;
				if(blk_top < window_top) {
					y = (window_top + window_height) - blk_height;
					this.Blk.offset({ left: x, top: y });
				}
			}
			//
			// attach event handlers
			if(!isEmpty(handlers)) {
				var n = handlers.length;
				for(var i = 0; i < n; i++) {
					var handler = handlers[i];
					if(!isNull(handler))
						this.Blk.delegate(handler.selector, handler.evt, handler.fn);
				}
			}
		}
	},
	//
	// Диалоги 
	//
	Dialog : {
		TYPE : {
			LOADABLE : 0x0001,
			INLINE   : 0x0002
		},
		List : new SList(),
		generateID : function() {
			var id = null;
			var i = 65;
			do {
				id = "UHTT_DIALOG_INSTANCE_" + String.fromCharCode(i++);
			} while(UHTT.Dialog.isExists(id));
			// TODO: ^^^^^^^^^^^^^^^^^^^^^^^
			return id;
		},
		Container : function() {
			var $container = $("#UHTT_DIALOGS_CONTAINER");
			if(!$container.exists()) {
				$("body").append('<div id="UHTT_DIALOGS_CONTAINER" style="display:none"></div>');
				$container = $("#UHTT_DIALOGS_CONTAINER");
			}
			return $container;
		}
	},
	/* Магазин */
	Store : {
		cfm_clearcart : "Вы действительно хотите удалить все заказы из корзины?",
		cfm_removecartplace : "Удалить место из заказа?",
		Preferences : {
			SID : 0,
			Name : ""
		},
		Selector : {
			ATTR : {
				attrGroup : 1,
				attrBrand : 2,
				attrName  : 3,
				attrClass : 4,
				attrTag   : 5
			},
			CLSF : {
				eKind  : 1,
				eGrade : 2,
				eAdd   : 3,
				eX     : 4,
				eY     : 5,
				eZ     : 6,
				eW     : 7,
				eAdd2  : 8
			},
			Rec : function(_obj) {
				this.ID = 0;
				this.Attr = 0;
				this.Clsf = 0;
				this.Title = "";
				this.Crit = "";
				this.Subcrit = "";
				this.Part = "";
				this.Values = null;
				if(_obj != null) {
					this.ID = _obj.ID;
					this.Attr = _obj.Attr;
					this.Clsf = _obj.Clsf;
					this.Title = _obj.Title;
					this.Crit = _obj.Crit;
					this.Subcrit = _obj.Subcrit;
					this.Part = _obj.Part;
					this.Values = _obj.Values;
				};
			}
		},
		Ctx : {
			ClassID   : 0,
			Selectors : new SList(),
			init : function(drawRoutine, args) {
				UHTT.BlockUI.block();
				var ok = false;
				var url = "/dispatcher/store/get_selectors?sid=" + UHTT.Store.Preferences.SID;
				jQuery.ajax(
					{
						url: url,
						async: false,
						success: function(data, status, hdr) 
						{
							try {
								if(!isEmpty(data)) {
									var hdr = UHTT.Util.JSON.parse(data);
									if(hdr != null) {
										UHTT.Store.Ctx.ClassID = hdr.Class;
										if(!isEmpty(hdr.Selectors)) {
											var n = hdr.Selectors.length;
											for(var i = 0; i < n; i++) {
												var sel = hdr.Selectors[i];
												if(sel != null) {
													var s = new UHTT.Store.Selector.Rec(sel);
													UHTT.Store.Ctx.Selectors.add(s);
													drawRoutine(s, args);
												}
											}
										}
									}
								}
								ok = true;
							} catch(e) { 
								console.log(e); 
							}
						},
						error: AJAX_ERR_HANDLER
					}
				).done(
					function() 
					{
						UHTT.BlockUI.unblock();
					}
				);
				return ok;
			},
		},
		//
		// Фильтр 
		//
		SelectorValue : function(s, value) 
		{
			this.S = s;
			this.Crit = s.Crit;
			this.Value = value;
		},
		Filter : function() 
		{
			this.PageNumber = 1;
			this.PageSize = 40;
			this.Selectors = new SList();
			this.setSelector = function(selector) {
				if(selector != null) {
					var idx = this.Selectors.getIdxByProperty("Crit", selector.S.Crit);
					if(idx > -1) {
						if(isEmpty(selector.Value)) {
							this.Selectors.remove(idx);
						}
						else {
							var s = this.Selectors.at(idx);
							s.Value = selector.Value;
						}
					}
					else {
						if(!isEmpty(selector.Value)) {
							this.Selectors.add(selector);
						}
					}
					this.PageNumber = 1;
				}
			};
		},
		OrderCtx : function(type) { // Структура заполняемая на форме ввода заказа
			this.BuyerInfo = {
				PersonID: 0,
				AddressID: 0,
				Name: "",
				Phone: "",
				Email: "",
				CityID: 0,
				ForcePerson: 0, // @v8.9.0 Если !0, то серверный модуль Java создает персоналию (в противном случае - автономный адрес)
				IsFullAddr: 0,   // @v8.3.11 Если !0, то поле Address содержит полное описание адреса (с городом, индексом и т.д.)
				Address: ""
			};
			this.PaymentSystem = {};
		},
		Order : function() { // Структура, получающая данные из OrderCtx и передаваемая на сервер
			this.Header = {
				Contact : "",
				Phone : "",
				Email: "",
				CityID : 0,
				LocFlags : 0, // @v8.3.11
				ForcePerson: 0, // @v8.9.0 Если !0, то серверный модуль Java создает персоналию (в противном случае - автономный адрес)
				Address : "",
				BuyerPersonID: 0, // @v8.9.0
				AddressID : 0
			};
			this.Lines = [];
		},
		Cart : {
			EVENTS : {
				EVT_ADD_ITEM    : 1,
				EVT_REMOVE_ITEM : 2,
				EVT_CLEAR       : 3
			},
			Item : function() {
				this.GoodsID = 0;
				this.GoodsName = "";
				this.Quantity = 0;
				this.Price = 0;
				this.Amount = 0;
			},
			/*
			    @sobolev
				Структура для хранения элемента корзины в cookie
				То же, что и Item, но без GoodsName (длинная корзина не умещается)
			*/
			CookieItem : function() {
				this.I = 0;
				this.Q = 0;
				this.P = 0.0;
				this.A = 0.0;				
			},
			Items : new SList(),
			EventHandlers : new SList(),
			addEventHandler : function(handler) {
				if(!isEmpty(handler) && typeof(handler) == "function")
					this.EventHandlers.addUnique({ fn: handler });
			},
			processEvent : function(type, ctx) {
				if(!this.EventHandlers.isEmpty()) {
					var n = this.EventHandlers.getCount();
					for(var i = 0; i < n; i++) {
						var handler = this.EventHandlers.at(i);
						if(!isEmpty(handler))
							handler.fn(type, ctx);
					}
				}
			},
			init : function() {
				var ary = null;
				try {
					ary = jQuery.parseJSON(UHTT.Util.Cookie.get("CARTDATA2_" + UHTT.Store.Preferences.SID));
				} catch(e) {
					UHTT.Util.Cookie.remove("CARTDATA2_" + UHTT.Store.Preferences.SID);
				}
				if(isArray(ary)) {
					var n = ary.length;
					for(var i = 0; i < n; i++) {
						var _item = ary[i];
						if(!isEmpty(_item.I) && !isEmpty(_item.Q) && !isEmpty(_item.P) && !isEmpty(_item.A)) {
							var item = new UHTT.Store.Cart.Item();
							item.GoodsID = _item.I;
							var uhtt_goods = UHTT.Store.getItem(_item.I.toString());
							item.GoodsName = uhtt_goods.Name;
							item.Quantity = _item.Q;
							item.Price = _item.P;
							item.Amount = _item.A;
							this.Items.add(item);
						}
					}
				}
				//
				// Cart 'change' event handler / write cart to cookie
				this.addEventHandler(function() {
					var _c = UHTT.Store.Cart.Items.getCount();
					if(_c > 0) {
						var cookie_ary = new SList;
						for(var i = 0; i < _c; i++) {
							var src_item = UHTT.Store.Cart.Items.getArray()[i];
							var dest_item = new UHTT.Store.Cart.CookieItem;
							if(isString(src_item.GoodsID))
								dest_item.I = parseInt(src_item.GoodsID);
							else
								dest_item.I = src_item.GoodsID;
							dest_item.Q = src_item.Quantity;
							dest_item.P = src_item.Price;
							dest_item.A = src_item.Amount;
							cookie_ary.add(dest_item);
						}
						UHTT.Util.Cookie.set("CARTDATA2_" + UHTT.Store.Preferences.SID, 
								UHTT.Util.JSON.toString(cookie_ary.getArray()), UHTT.Util.Cookie.expireByHours(1), "/");						
					}
					else {
						UHTT.Util.Cookie.remove("CARTDATA2_" + UHTT.Store.Preferences.SID);
					}
				});
			},
			add : function(id, qtty) {
				var goods = UHTT.Store.getItem(id);
				if(goods != null) {
					var item = this.Items.getObjByProperty("GoodsID", id);
					if(item != null) {
						item.Quantity += qtty;
						item.Amount = goods.Value * item.Quantity;
					}
					else {
						item = new UHTT.Store.Cart.Item();
						item.GoodsID = id;
						item.GoodsName = goods.Name;
						item.Quantity = qtty;
						item.Price = goods.Value;
						item.Amount = goods.Value * qtty;
						this.Items.add(item);
					}
					UHTT.Messenger.show("Товар '" + goods.Name + "' добавлен в корзину" + 
							"<br>Количество: " + qtty + 
							"<br>На сумму: " + Number(item.Amount).toFixed(2),
							"/rsrc/images/add2cart_64.png");
					this.processEvent(UHTT.Store.Cart.EVENTS.EVT_ADD_ITEM, { itemID: id, qtty: qtty });
				}
			},
			remove : function(id) {
				var idx = this.Items.getIdxByProperty("GoodsID", id);
				if(idx > -1) {
					var item = this.Items.at(idx);
					if(item != null) {
						//item.Quantity--;
						//item.Amount = item.Price * item.Quantity;
						//if(item.Quantity <= 0)
						//	this.Items.remove(idx);
						this.Items.remove(idx);
						this.processEvent(UHTT.Store.Cart.EVENTS.EVT_REMOVE_ITEM, { itemID: id });
					}
				}
			},
			removeAll : function(doConfirm) {
				if(doConfirm != true || confirm(UHTT.Store.cfm_clearcart)) {
					this.Items.flush();
					this.processEvent(UHTT.Store.Cart.EVENTS.EVT_CLEAR, null);
				}
			}
		},
		TSessCart : {
			EVENTS : {
				EVT_ADD_ITEM    : 1,
				EVT_REMOVE_ITEM : 2,
				EVT_CLEAR       : 3
			},
			Item : function() {
				this.TSessID = 0;
				this.PlaceCode = "";
				this.PrcID = 0;				
				this.Quantity = 0; 
				this.Price = 0;
				this.Amount = 0;
			},
			CookieItem : function() {
				this.S = 0; // TSessionID
				this.L = ""; // Place Code
				this.R = 0; // ProcessorID				
				this.Q = 0; 
				this.P = 0.0;
				this.A = 0.0;				
			},
			Items : new SList(),
			EventHandlers : new SList(),
			addEventHandler : function(handler) {
				if(!isEmpty(handler) && typeof(handler) == "function")
					this.EventHandlers.addUnique({ fn: handler });
			},
			processEvent : function(type, ctx) {
				if(!this.EventHandlers.isEmpty()) {
					var n = this.EventHandlers.getCount();
					for(var i = 0; i < n; i++) {
						var handler = this.EventHandlers.at(i);
						if(!isEmpty(handler))
							handler.fn(type, ctx);
					}
				}
			},
			init : function() 
			{
				var ary = null;
				try {
					ary = jQuery.parseJSON(UHTT.Util.Cookie.get("TSESSCARTDATA" + UHTT.Store.Preferences.SID));
				} 
				catch(e) {
					UHTT.Util.Cookie.remove("TSESSCARTDATA" + UHTT.Store.Preferences.SID);
				}
				if(isArray(ary)) {
					var n = ary.length;
					for(var i = 0; i < n; i++) {
						var _item = ary[i];
						if(!isEmpty(_item.S) && !isEmpty(_item.L) && !isEmpty(_item.Q)) {
							var item = new UHTT.Store.TSessCart.Item();
							item.TSessID = _item.S;
							item.PlaceCode = _item.L;
							item.PrcID = _item.R;							
							item.Quantity = _item.Q;
							item.Price = _item.P;
							item.Amount = _item.A;
							this.Items.add(item);
						}
					}
				}
				//
				// Cart 'change' event handler / write cart to cookie
				//
				this.addEventHandler(
					function() {
						var _c = UHTT.Store.TSessCart.Items.getCount();
						if(_c > 0) {
							var cookie_ary = new SList;
							for(var i = 0; i < _c; i++) {
								var src_item = UHTT.Store.TSessCart.Items.getArray()[i];
								var dest_item = new UHTT.Store.TSessCart.CookieItem;
								dest_item.S = src_item.TSessID;
								dest_item.L = src_item.PlaceCode;
								dest_item.R = src_item.PrcID;
								dest_item.Q = src_item.Quantity;
								dest_item.P = src_item.Price;
								dest_item.A = src_item.Amount;
								cookie_ary.add(dest_item);
							}
							UHTT.Util.Cookie.set("TSESSCARTDATA" + UHTT.Store.Preferences.SID, 
								UHTT.Util.JSON.toString(cookie_ary.getArray()), UHTT.Util.Cookie.expireByHours(1), "/");						
						}
						else
							UHTT.Util.Cookie.remove("TSESSCARTDATA" + UHTT.Store.Preferences.SID);
					}
				);
			},
			hasPlace : function(tsesID, placeCode) 
			{
				var item = this.Items.getObjByProperty2("TSessID", tsesID, "PlaceCode", placeCode);
				return (item != null) ? 1 : 0;
			},
			add : function(tsesID, placeCode) {
				var tses_item = UHTT.Store.queryTSesItem(tsesID, placeCode);
				if(tses_item != null) {
					if(tses_item.PlaceStatus > 0) {
						var item = this.Items.getObjByProperty2("TSessID", tsesID, "PlaceCode", placeCode);
						if(item != null) {
							//UHTT.Messenger.show("Место " + placeCode + " уже в корзине", "/rsrc/images/error_48.png");						
							if(confirm(UHTT.Store.cfm_removecartplace)) {
								this.remove(tsesID, placeCode);
							}
						}
						else {
							item = new UHTT.Store.TSessCart.Item();
							item.TSessID = tsesID;
							item.PlaceCode = placeCode;
							item.PrcID = tses_item.PrcID;				
							item.Quantity = 1; 
							item.Price = tses_item.PlacePrice;
							item.Amount = tses_item.PlacePrice;						
							this.Items.add(item);
							UHTT.Messenger.show("Регистрация <br>" + tses_item.PrcName + "<br>" + tses_item.Descr + "<br>" + "добавлена в корзину" + 
									"<br>На сумму: " + Number(item.Amount).toFixed(2),
									"/rsrc/images/add2cart_64.png");
							this.processEvent(UHTT.Store.TSessCart.EVENTS.EVT_ADD_ITEM, { tsesID: tsesID, placeCode: placeCode });						
						}
					}
					else if(tses_item.PlaceStatus < 0) {
						UHTT.Messenger.show("Место " + placeCode + " занято", "/rsrc/images/error_48.png");						
					}
					else {
						UHTT.Messenger.show("Ошибка идентификации места " + placeCode, "/rsrc/images/error_48.png");						
					}
				}
			},
			remove : function(tsesID, placeCode) 
			{
				var idx = this.Items.getIdxByProperty2("TSessID", tsesID, "PlaceCode", placeCode);
				if(idx > -1) {
					var item = this.Items.at(idx);
					if(item != null) {
						//item.Quantity--;
						//item.Amount = item.Price * item.Quantity;
						//if(item.Quantity <= 0)
						//	this.Items.remove(idx);
						this.Items.remove(idx);
						this.processEvent(UHTT.Store.TSessCart.EVENTS.EVT_REMOVE_ITEM, { tsesID: tsesID, placeCode: placeCode });
					}
				}
			},
			removeAll : function(doConfirm) 
			{
				if(doConfirm != true || confirm(UHTT.Store.cfm_clearcart)) {
					this.Items.flush();
					this.processEvent(UHTT.Store.TSessCart.EVENTS.EVT_CLEAR, null);
				}
			},
			//
			// storeSymb: @#req символ магазина, для которого оформляется заказ
			// options: параметры диалога заказа
			//
			commitOrder : function(storeSymb, options)
			{
				if(isNull(storeSymb)) {
					UHTT.Messenger.show("Внутренняя ошибка: вызов commitOrder не определил аргумент storeSymb", "/rsrc/images/error_48.png");							
				}
				else {
					var user = UHTT.UserUtil.getCurrentUser();
					var org = (!isNull(user) && !isEmpty(user.RelList)) ? true : false;
					if(!this.Items.isEmpty()) {
						var view = UHTT.Workbook.getContent("STORE_ORDER_FRM", { store: storeSymb });
						var opts = (!isNull(options)) ? options : {
							modal: true, width: 700, height: 520, position: "center", title: "Оформление заказа на мероприятие"
						};
						new SDialog("STORE_ORDER_DLG", view, opts, UHTT.Dialog.TYPE.INLINE);
					}
				}
			}
		},		
		Catalog : {
			DefaultTemplate : null,
			Initialized : false,
			ItemsCount : 0,
			$View : null,
			init : function($view, defaultTemplate) 
			{
				if(!isEmpty($view) && $view.exists() && !isEmpty(defaultTemplate)) {
					this.$View = $view;
					this.DefaultTemplate = defaultTemplate;
					this.Initialized = true;
					return true;
				}
				return false;
			},
			draw : function(F, mode) 
			{
				if(this.Initialized) {
					if(F instanceof UHTT.Store.Filter) {
						UHTT.BlockUI.block();
						var url = "/dispatcher/store/draw_catalog?sid=" + UHTT.Store.Preferences.SID;
						var has_class = false;
						var selectors_count = F.Selectors.getCount();
						for(var i = 0; i < selectors_count; i++) {
							var s = F.Selectors.at(i);
							if(s != null) {
								if(s.S.Attr == UHTT.Store.Selector.ATTR.attrClass) {
									has_class = true;
									break;
								}
							}
						}
						if(has_class)
							url += "&class=" + UHTT.Store.Ctx.ClassID;
						url += "&crits=" + UHTT.Util.JSON.toStringSkipItem(F.Selectors.getArray(), "Values");
						url += "&page=" + F.PageNumber;
						url += "&view_mode=" + (!isEmpty(mode) ? mode : this.DefaultTemplate);
						console.log(url);
						jQuery.ajax(
							{
								url: url,
								async: false,
								success: function(data, status, hdr) 
								{
									UHTT.Store.Catalog.ItemsCount = parseInt(hdr.getResponseHeader("X-UHTT-ViewItemsCount"));
									UHTT.Store.Catalog.$View.html(data);
								},
								error: AJAX_ERR_HANDLER
							}
						).done(
							function() 
							{
								UHTT.BlockUI.unblock();
							}
						);
					}
				}
			}
		},
		getItem : function(goods_id) {
			var item = null;
			var url = "/dispatcher/store/get_item?sid=" + UHTT.Store.Preferences.SID + "&goods_id=" + goods_id;
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: function(data, status, hdr) 
					{
						if(!isEmpty(data))
							try { item = UHTT.Util.JSON.parse(data); } catch(e) {}
					},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() {
				}
			);
			return item;
		},
		helperQueryTSesItem : function(tses_id, place_code) 
		{
			var result = null;
			var url = "/dispatcher/store/get_tses_item?sid=" + UHTT.Store.Preferences.SID + "&tses_id=" + tses_id;
			if(isString(place_code)) {
				url = url + "&place_code=" + place_code;
			}
			//
			// Значения поля status для placeInfo:
			//   0 - место не идентифицировано
			//   1 - место свободно
			//  -1 - место занято
			//
			// placeInfo { place_code, price, status, description }
			//			
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: function(data, status, hdr) {
						if(!isEmpty(data))
							try { 
								result = UHTT.Util.JSON.parse(data); 
							} 
							catch(e) {
							}
					},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() {
				}
			);
			return result;
		},	
		queryTSesItem : function(tses_id, place_code) 
		{
			return this.helperQueryTSesItem(tses_id, place_code);
		},
		queryTSesCipStatusArray : function(tses_id) 
		{
			return this.helperQueryTSesItem(tses_id, "all");
		},		
		//
		// Получает статус места по коду placeCode из массива placeStatusArray
		// Структура элемента массива: { code: '$ps.Code', status: $ps.Status, price: $ps.Price, descr: '$ps.Descr' }
		// Этот массив должен быть определен на листе раскладки зрительного зала.
		//
		getPlaceStatusItemByCode : function(placeStatusArray, placeCode)
		{
			var L = placeStatusArray.length;
			for(var i = 0; i < L; i++) {
				var item = placeStatusArray[i];
				if(item.code == placeCode) 
					return item;
			}
			return null;
		},
		//
		// Обновляет содержимое массива placeStatusArray по техсессии с идентификатором tses_id запросом к серверу.
		// Структура элемента массива: { code: '$ps.Code', status: $ps.Status, price: $ps.Price, descr: '$ps.Descr' }
		// Этот массив должен быть определен на листе раскладки зрительного зала.
		//		
		refreshPlaceStatusArray : function(placeStatusArray, tses_id)
		{
			var L = placeStatusArray.length;
			if(L > 0) {
				var status_ary = this.queryTSesCipStatusArray(tses_id);
				if(isArray(status_ary)) {
					var sc = status_ary.length;
					for(var j = 0; j < sc; j++) {
						var status_item = status_ary[j];
						for(var i = 0; i < L; i++) {
							var item = placeStatusArray[i];
							if(item.code == status_item.PlaceCode && item.status != status_item.PlaceStatus)  {
								item.status = status_item.PlaceStatus;
								item.price = status_item.PlacePrice;
							}
						}
					}
				}
			}
		},		
		getRest : function(goods_id) 
		{
			var val = "";
			var url = "/dispatcher/store/get_rest?sid=" + UHTT.Store.Preferences.SID + "&goods_id=" + goods_id;
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: function(data, status, hdr) {
						val = data;
					},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() {
				}
			);
			return val;
		},
		getFilterData : function() 
		{
			UHTT.BlockUI.block();
			var _data = null;
			var url = "/dispatcher/store/get_filter_data?sid=" + UHTT.Store.Preferences.SID;
			jQuery.ajax({
				url: url,
				async: false,
				success: function(data, status, hdr) {
					try {
						_data = UHTT.Util.JSON.parse(data);
					} catch(e) {}
				},
				error: AJAX_ERR_HANDLER
			}).done(function() {
				UHTT.BlockUI.unblock();
			});
			return _data;
		},
		checkout : function(ctx, success_handler, error_handler, done_handler) {
			if(!isEmpty(ctx) && (ctx instanceof UHTT.Store.OrderCtx) && !UHTT.Store.Cart.Items.isEmpty()) {
				var order = new UHTT.Store.Order();
				// set order header
				order.Header.Contact = ctx.BuyerInfo.Name;
				order.Header.Phone = ctx.BuyerInfo.Phone;
				order.Header.Email = ctx.BuyerInfo.Email;
				order.Header.CityID = ctx.BuyerInfo.CityID;
				order.Header.LocFlags = ctx.BuyerInfo.IsFullAddr ? 2 : 0; // @v8.3.11
				order.Header.ForcePerson = ctx.BuyerInfo.ForcePerson; // @8.9.0
				order.Header.Address = ctx.BuyerInfo.Address;
				order.Header.AddressID = ctx.BuyerInfo.AddressID;
				order.Header.BuyerPersonID = ctx.BuyerInfo.PersonID; // @v8.9.0
				// insert order lines
				{
					var n = UHTT.Store.Cart.Items.getCount();
					for(var i = 0; i < n; i++) {
						var item = UHTT.Store.Cart.Items.at(i);
						if(item != null)
							order.Lines.push({ "GoodsID":item.GoodsID, "Quantity":item.Quantity, "Price":item.Price });
					}
				}
				// send request
				var url = "/dispatcher/store/checkout?sid=" + UHTT.Store.Preferences.SID + "&order=" + UHTT.Util.JSON.toString(order);
				jQuery.ajax({
					url: url,
					async: false,
					success: success_handler,
					error:   error_handler
				}).done(done_handler);
			}
		},
		TSessCipOpBlock : function() { // Структура, заполняемая на фронте, для вызова специализированных функций регистрации по техсессиям
			this.Op = ""; // CheckIn || Cancel
			this.TSessID = 0;
			this.CipID = 0;
			this.PlaceCode = "";
			this.PinCode = "";
		},		
		TSessCipCheckIn : function(ctx, success_handler, error_handler, done_handler) 
		{
			if(!isEmpty(ctx) && (ctx instanceof UHTT.Store.TSessCipOpBlock)) {
				var url = "/dispatcher/store/checkout?sid=" + UHTT.Store.Preferences.SID + "&tsessCipOp=" + UHTT.Util.JSON.toString(ctx);
				jQuery.ajax(
					{
						url: url,
						async: false,
						success: success_handler,
						error:   error_handler
					}
				).done(done_handler);
			}		
		},
		TSessCipCancel : function(ctx, success_handler, error_handler, done_handler) 
		{
			if(!isEmpty(ctx) && (ctx instanceof UHTT.Store.TSessCipOpBlock)) {
				var url = "/dispatcher/store/checkout?sid=" + UHTT.Store.Preferences.SID + "&tsessCipOp=" + UHTT.Util.JSON.toString(ctx);
				jQuery.ajax(
					{
						url: url,
						async: false,
						success: success_handler,
						error:   error_handler
					}
				).done(done_handler);
			}				
		},
		checkoutTSess : function(ctx, success_handler, error_handler, done_handler) 
		{
			if(!isEmpty(ctx) && (ctx instanceof UHTT.Store.OrderCtx) && !UHTT.Store.TSessCart.Items.isEmpty()) {
				var order = new UHTT.Store.Order();
				// set order header
				order.Header.Contact = ctx.BuyerInfo.Name;
				order.Header.Phone = ctx.BuyerInfo.Phone;
				order.Header.Email = ctx.BuyerInfo.Email;
				order.Header.CityID = ctx.BuyerInfo.CityID;
				order.Header.LocFlags = ctx.BuyerInfo.IsFullAddr ? 2 : 0; // @v8.3.11
				order.Header.ForcePerson = ctx.BuyerInfo.ForcePerson; // @8.9.0
				order.Header.Address = ctx.BuyerInfo.Address;
				order.Header.AddressID = ctx.BuyerInfo.AddressID;
				order.Header.BuyerPersonID = ctx.BuyerInfo.PersonID; // @v8.9.0
				// insert order lines
				{
					var n = UHTT.Store.TSessCart.Items.getCount();
					for(var i = 0; i < n; i++) {
						var item = UHTT.Store.TSessCart.Items.at(i);
						if(item != null)
							order.Lines.push({ "TSessID":item.TSessID, "PlaceCode":item.PlaceCode, "Quantity":item.Quantity, "Price":item.Price });
					}
				}
				// send request
				var url = "/dispatcher/store/checkout?sid=" + UHTT.Store.Preferences.SID + "&order=" + UHTT.Util.JSON.toString(order);
				jQuery.ajax(
					{
						url: url,
						async: false,
						success: success_handler,
						error:   error_handler
					}
				).done(done_handler);
			}
		},		
		getStoreList : function() {
			UHTT.BlockUI.block();
			var _data = null;
			var url = "/dispatcher/store/get_store_list";
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: function(data, status, hdr) {
						try {
							_data = UHTT.Util.JSON.parse(data);
						} catch(e) {}
					},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() 
				{
					UHTT.BlockUI.unblock();
				}
			);
			return _data;
		},
		getOrders : function(store_id) {
			UHTT.BlockUI.block();
			var _data = null;
			var url = "/dispatcher/store/get_orders?store_id=" + store_id;
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: function(data, status, hdr) {
						try {
							_data = UHTT.Util.JSON.parse(data);
						} catch(e) {}
					},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() 
				{
					UHTT.BlockUI.unblock();
				}
			);
			return _data;
		},
		helperGetPlaceCodeRow : function(rp)
		{
			if(isArray(rp)) {
				if(rp.length == 2)
					return rp[0];
				else if(rp.length == 3)
					return rp[1];
			}
			return null;
		},
		helperGetPlaceCodeColumn : function(rp)
		{
			if(isArray(rp)) {
				if(rp.length == 2)
					return rp[1];
				else if(rp.length == 3)
					return rp[2];
			}
			return null;		
		},				
		//
		// Возвращает номер ряда из кода места.
		//
		getPlaceCodeRow : function(placeCode)
		{
			var rp = placeCode.split('-');
			return this.helperGetPlaceCodeRow(rp);
		},
		//
		// Возвращает номер места из кода места.
		//		
		getPlaceCodeColumn : function(placeCode)
		{
			var rp = placeCode.split('-');
			return this.helperGetPlaceCodeColumn(rp);		
		},		
		formatPlaceCode : function(placeCode)
		{
			var rp = placeCode.split('-');
			var row = this.helperGetPlaceCodeRow(rp);
			var col = this.helperGetPlaceCodeColumn(rp);
			if(!isNull(row) && !isNull(col)) 
				return "Ряд" + " " + row + " " + "Место" + " " + col;
			else
				return placeCode;
		}
	},
	//
	// Браузер
	//
	Browser : {
		Page : function(title, content) {
			this.Title = title;
			this.Content = content;
		},
		Initialized : false,
		Pages : new SList(),
		CurrentPageIdx : -1,
		/* jQuery objects */
		$View : null,
		$TitleBlk : null,
		$PageTitle : null,
		$ControlBlk : null,
		$BackwardLink : null,
		$ForwardLink : null,
		$CloseLink : null,
		$Box : null,
		/* Functions */
		display : function() {
			if(this.Initialized) {
				if(this.CurrentPageIdx < 0) {
					/* Cleanup */
					this.$TitleBlk.hide();
					this.$Box.hide();
					this.$Box.html("");
				}
				else {
					var page = this.Pages.at(this.CurrentPageIdx);
					if(page != null) {
						this.$TitleBlk.show();
						if(this.Pages.getCount() > 0) {
							if(this.CurrentPageIdx > 0)
								this.$BackwardLink.removeClass("uhtt-browser-nav-backward-link-disabled");
							else
								this.$BackwardLink.addClass("uhtt-browser-nav-backward-link-disabled");
							if(this.CurrentPageIdx < this.Pages.getCount() - 1 /* last idx */)
								this.$ForwardLink.removeClass("uhtt-browser-nav-forward-link-disabled");
							else
								this.$ForwardLink.addClass("uhtt-browser-nav-forward-link-disabled");
						}
						if(!isEmpty(page.Title))
							this.$PageTitle.html(page.Title);
						else
							this.$PageTitle.html("Страница " + this.CurrentPageIdx + 1);
						this.$Box.fadeOut(100, function() {
							UHTT.Browser.$Box.html(page.Content);
							UHTT.Browser.$Box.fadeIn(100);
						});
					}
				}
			}
		},
		addPage : function(page) {
			if(page instanceof(UHTT.Browser.Page)) {
				var idx = this.CurrentPageIdx + 1;
				while(this.Pages.at(idx) != null)
					this.Pages.remove(idx);
				this.Pages.add(page);
				this.CurrentPageIdx = idx;
				this.display();
			}
		},
		removePage : function() {
			if(this.CurrentPageIdx > -1) {
				this.Pages.remove(this.CurrentPageIdx);
				if(this.Pages.isEmpty())
					this.CurrentPageIdx = -1;
				else if(this.CurrentPageIdx > 0)
					this.CurrentPageIdx--;
				this.display();
			}
		},
		goForward : function() {
			if(this.CurrentPageIdx < this.Pages.getCount() - 1) {
				this.CurrentPageIdx++;
				this.display();
			}
		},
		goBackward : function() {
			if(this.CurrentPageIdx > 0) {
				this.CurrentPageIdx--;
				this.display();
			}
		},
		show : function(content, title) {
			this.addPage(new UHTT.Browser.Page(title, content));
		},
		init : function(id) {
			if(!isEmpty(id)) {
				this.$View = $("#" + id);
				if(this.$View.exists()) {
					this.$TitleBlk = $('<div class="uhtt-browser-title-blk"></div>').appendTo(this.$View);
					this.$PageTitle = $('<span></span>').appendTo(this.$TitleBlk);
					this.$ControlBlk = 
						$('<div class="uhtt-browser-control-blk"></div>').appendTo(this.$TitleBlk);
					this.$BackwardLink = 
						$('<a class="uhtt-browser-nav-link uhtt-browser-nav-backward-link"></a>').appendTo(this.$ControlBlk);
					this.$ForwardLink =
						$('<a class="uhtt-browser-nav-link uhtt-browser-nav-forward-link"></a>').appendTo(this.$ControlBlk);
					this.$CloseLink = 
						$('<a class="uhtt-browser-nav-link uhtt-browser-nav-close-link"></a>').appendTo(this.$ControlBlk);
					this.$Box = $('<div class="sprawling uhtt-browser-box"></div>').appendTo(this.$View);
					this.$BackwardLink.Evt("click", function() {
						UHTT.Browser.goBackward();
					});
					this.$ForwardLink.Evt("click", function() {
						UHTT.Browser.goForward();
					});
					this.$CloseLink.Evt("click", function() {
						UHTT.Browser.removePage();
					});
					this.Initialized = true;
					this.display();
				}
			}
		}
	},
	DC : {
		Item : function(json) {
			this.ID = 0;
			this.PID = 0;
			this.Key = "";
			this.Revision = 0;
			this.Name = "";
			this.Ts = "";
			this.Size = 0;
			this.Memo = "";
			if(!isEmpty(json)) {
				var _obj = UHTT.Util.JSON.parse(json);
				if(_obj != null) {
					this.ID = _obj.ID;
					this.PID = _obj.PID;
					this.Key = _obj.Key.unescapeHtml();
					this.Revision = _obj.Revision;
					this.Name = _obj.Name.unescapeHtml();
					this.Ts = _obj.Ts.unescapeHtml();
					this.Size = _obj.Size;
					this.Memo = _obj.Memo.unescapeHtml();
				}
			}
		},
		Editor : {
			/* Change handler */
			_Handler : null,
			/* Args */
			_Args : new SList(),
			/* Access functions */
			getArgs : function() { return this._Args; },
			createFolder : function(name, parentID, success_handler, error_handler, done_handler) {
				var url = "/dispatcher/dc/create_folder?name=" + encodeURIComponent(name) + "&pid=" + parentID;
				jQuery.ajax({
					url: url,
					async: false,
					success: success_handler,
					error: error_handler
				}).done(done_handler);
			},
			deleteFolder : function(id, success_handler, error_handler, done_handler) {
				var url = "/dispatcher/dc/remove_folder?id=" + id;
				jQuery.ajax({
					url: url,
					async: false,
					success: success_handler,
					error: error_handler
				}).done(done_handler);
			},
			openCreateFileDialog : function(parentID) {
				this._Args.flush();
				this._Args.add({ "ParentID": parentID });
				new SDialog("UHTT_DC_CREATE_FILE_FORM", "GETTDDO FRM_DC_CREATE_FILE", { modal:true, position:"center"});
			},
			openUpdateFileDialog : function(id, parentID, type, title, key, memo) {
				this._Args.flush();
				this._Args.add({ "ID": id });
				this._Args.add({ "ParentID": parentID });
				this._Args.add({ "Type": type });
				this._Args.add({ "Title": title });
				this._Args.add({ "Key": key });
				this._Args.add({ "Memo": memo });
				new SDialog("UHTT_DC_UPDATE_FILE_FORM", "GETTDDO FRM_DC_UPDATE_FILE", { modal:true, position:"center" });
			},
			openAddVersionDialog : function(id) {
				this._Args.flush();
				this._Args.add({ "ID": id });
				new SDialog("UHTT_DC_ADD_FILE_VERSION_FORM", "GETTDDO FRM_DC_ADD_FILE_VERSION", { modal:true, position:"center" });
			},
			openFileVersionViewForm : function(id) {
				this._Args.flush();
				this._Args.add({ "ID": id });
				new SDialog("UHTT_DC_FILE_VERSION_VIEW_FORM", "GETTDDO FRM_DC_FILE_VERSION_VIEW", { modal:true, position:"center" });
			},
			openRightsEditorDialog : function(id, name) {
				this._Args.flush();
				this._Args.add({ "ID": id });
				this._Args.add({ "Name": name });
				new SDialog("UHTT_DC_ITEM_RIGHTS_EDITOR_FORM", "GETTDDO FRM_DC_ITEM_RIGHTS", { modal:true, position:"center" });
			},
			deleteFile : function(id, success_handler, error_handler, done_handler) {
				var url = "/dispatcher/dc/remove_file?id=" + id;
				jQuery.ajax({
					url: url,
					async: false,
					success: success_handler,
					error: error_handler
				}).done(done_handler);
			},
			deleteVersion : function(id, rev, success_handler, error_handler, done_handler) 
			{
				var url = "/dispatcher/dc/remove_file_version?id=" + id + "&rev=" + rev;
				jQuery.ajax(
					{
						url: url,
						async: false,
						success: success_handler,
						error: error_handler
					}
				).done(done_handler);
			},
			renameItem : function(id, name, success_handler, error_handler, done_handler) 
			{
				var url = "/dispatcher/dc/rename_item?id=" + id + "&name=" + name;
				jQuery.ajax(
					{
						url: url,
						async: false,
						success: success_handler,
						error: error_handler
					}
				).done(done_handler);
			},
			clear : function() 
			{
				this._Handler = null;
			}
		},
		getFolderHierarchy : function() {
			var hierarchy = null;
			var url = "/dispatcher/dc/tree";
			jQuery.ajax({
				url: url,
				async: false,
				success: function(data, status, hdr) {
					hierarchy = UHTT.Util.JSON.parse(data);
				},
				error: AJAX_ERR_HANDLER
			}).done(function() {
			});
			return hierarchy;
		},
		getFolderChildren : function(id) {
			var children = null;
			var url = "/dispatcher/dc/ls?id=" + id;
			jQuery.ajax({
				url: url,
				async: false,
				success: function(data, status, hdr) {
					children = data;
				},
				error: AJAX_ERR_HANDLER
			}).done(function() {
			});
			return children;
		},
		getVersionList : function(id) {
			var list = null;
			var url = "/dispatcher/dc/vlst?id=" + id;
			jQuery.ajax({
				url: url,
				async: false,
				success: function(data, status, hdr) {
					list = new SList(UHTT.Util.JSON.parse(data));
				},
				error: AJAX_ERR_HANDLER
			}).done(function() {
			});
			return list;
		},
		getItemRights : function(id) {
			var rt_descr = null;
			var url = "/dispatcher/dc/get_rights?id=" + id;
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: function(data, status, hdr) 
					{
						rt_descr = UHTT.Util.JSON.parse(data);
					},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() 
				{
				}
			);
			return rt_descr;
		},
		setItemRights : function(rt_descr) {
			var ok = false;
			var url = "/dispatcher/dc/set_rights?rt_descr=" + UHTT.Util.JSON.toString(rt_descr);
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: function(data, status, hdr) {
						ok = true;
					},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() 
				{
				}
			);
			return ok;
		},
		getAvailableDownloads : function() {
			var list = null;
			var url = "/dispatcher/dc/avail_downloads";
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: function(data, status, hdr) 
					{
						list = new SList(UHTT.Util.JSON.parse(data));
					},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() 
				{
				}
			);
			return list;
		},
		download : function(url) 
		{
			$.fileDownload(url, 
				{
					successCallback: function (url) {},
					failCallback: function (html, url) 
					{
						UHTT.Messenger.show("Ошибка: " + html, "/standard-theme/images/uhtt/error_48.png");
					}
				}
			);
		}
	},
	//
	// TODO: Goods mark
	Goods : {
		Barcode : {
			Rec : function(_obj) {
				this.Code = "";
				this.Package = 0;
				if(!isNull(_obj)) {
					if("Code" in _obj)
						this.Code = _obj.Code.unescapeHtml();
					if("Package" in _obj)
						this.Package = _obj.Package;
				}
			},
			Editor : _extends(BaseEditor, {
				edit : function(idx) {
					if(isNull(UHTT.Goods.Editor.Object))
						throw new Error("Error: Goods editor not init");
					this.Object = null;
					if(idx > -1) {
						if(!isEmpty(UHTT.Goods.Editor.Object.BarcodeList) && (idx < UHTT.Goods.Editor.Object.BarcodeList.length))
							this.Object = clone(UHTT.Goods.Editor.Object.BarcodeList[idx]);
						if(isNull(this.Object))
							idx = -1;
					}
					this.IsEditing = (this.Object != null);
					if(this.Object == null)
						this.Object = new UHTT.Goods.Barcode.Rec();
					this.Args.flush();
					this.Args.add({ Idx: idx });
					new SDialog("UHTT_BARCODE_EDITOR_FRM", UHTT.Content.get("BARCODE_EDITOR", {}), { modal: true, position: "center" }, UHTT.Dialog.TYPE.INLINE);
				}
			})
		},
		Rec : function(json) {
			this.ID = 0;
			this.Name = "";
			this.GroupID = 0;
			this.BrandID = 0;
			this.ManufID = 0;
			this.TypeID = 0;
			this.ClsID = 0;
			this.TaxGrpID = 0;
			this.UnitID = 0;
			this.PhUnitID = 0;
			this.PhPerUnit = 0.0;
			this.Brutto = 0.0;
			this.Length = 0;
			this.Width = 0;
			this.Height = 0;
			this.Package = 0.0;
			this.ExpiryPeriod = 0;
			this.Storage = "";
			this.Standard = "";
			this.Ingred = "";
			this.Energy = "";
			this.Usage = "";
			this.OKOF = "";
			this.BarcodeList = [];
			if(!isEmpty(json)) {
				var _obj = UHTT.Util.JSON.parse(json);
				if(_obj != null) {
					this.ID = _obj.ID;
					this.Name = _obj.Name.unescapeHtml();
					this.GroupID = _obj.GroupID;
					this.BrandID = _obj.BrandID;
					this.ManufID = _obj.ManufID;
					this.TypeID = _obj.TypeID;
					this.ClsID = _obj.ClsID;
					this.TaxGrpID = _obj.TaxGrpID;
					this.UnitID = _obj.UnitID;
					this.PhUnitID = _obj.PhUnitID;
					this.PhPerUnit = _obj.PhPerUnit;
					this.Brutto = _obj.Brutto;
					this.Length = _obj.Length;
					this.Width = _obj.Width;
					this.Height = _obj.Height;
					this.Package = _obj.Package;
					this.ExpiryPeriod = _obj.ExpiryPeriod;
					this.Storage = _obj.Storage.unescapeHtml();
					this.Standard = _obj.Standard.unescapeHtml();
					this.Ingred = _obj.Ingred.unescapeHtml();
					this.Energy = _obj.Energy.unescapeHtml();
					this.Usage = _obj.Usage.unescapeHtml();
					this.OKOF = _obj.OKOF.unescapeHtml();
					if(!isEmpty(_obj.BarcodeList)) {
						for(var i = 0, len = _obj.BarcodeList.length; i < len; i++) {
							var _item = _obj.BarcodeList[i];
							if(!isNull(_item)) {
								this.BarcodeList.push(new UHTT.Goods.Barcode.Rec(_item));
							}
						}
					}
				}
			}
		},
		Editor : _extends(BaseEditor, {
			setBarcode : function(idx, barcode) {
				var ok = true;
				if(!(barcode instanceof UHTT.Goods.Barcode.Rec))
					return false;
				if(!isNull(this.Object) && !isNull(this.Object.BarcodeList)) {
					for(var i = 0, n = this.Object.BarcodeList.length; i < n; i++) {
						var item = this.Object.BarcodeList[i];
						if(!isNull(item)) {
							if(item.Code === barcode.Code) {
								if(idx < 0) {  // создание
									ok = false;
									break;
								}
								else {  // изменение
									if(idx != i) {
										ok = false;
										break;
									}
								}
							}
						}
					}
					if(ok) {
						if(idx < 0)  // создание
							this.Object.BarcodeList.push(barcode);
						else {  // изменение
							if(idx < this.Object.BarcodeList.length)
								this.Object.BarcodeList[idx] = barcode;
						}
						this.processEvent();
					}
				}
				return ok;
			},
			removeBarcode : function(idx) {
				if(!isNull(this.Object) && !isNull(this.Object.BarcodeList)) {
					if((idx > -1) && (idx < this.Object.BarcodeList.length)) {
						this.Object.BarcodeList.splice(idx, 1);
						this.processEvent();
					}
				}
			},
			edit : function(id) {
				this.Object = null;
				if(id > 0)
					this.Object = UHTT.Goods.search(id);
				this.IsEditing = (this.Object != null);
				if(this.Object == null)
					this.Object = new UHTT.Goods.Rec();
				new SDialog("UHTT_GOODS_EDITOR_FRM", UHTT.Content.get("GOODS_EDITOR", {}), { modal: true, position: "center" }, UHTT.Dialog.TYPE.INLINE);
			}
		}),
		Filter : function(template) {
			/* ----------------------------------------------------------------- */
			/* Критерии */
			this.Name = "";
			this.Barcode = "";
			this.BrandID = 0;
			this.ManufID = 0;
			this.Groups = new SList();
			/* Шаблон вывода данных */
			this.Template = isEmpty(template) ? "VIEW_GOODS" : template;
			/* Обработчик события изменения фильтра */
			this.ChangeHandler = null;
			/* ----------------------------------------------------------------- */
			/* Функции */
			this.setName = function(name) { this.Name = name; };
			this.setBarcode = function(code) { this.Barcode = code; };
			this.setBrandID = function(id) { this.BrandID = id; };
			this.setManufID = function(id) { this.ManufID = id; };
			this.setGroups = function(groups) { this.Groups = groups; };
			this.setTemplate = function(template) { this.Template = template; };
			this.setChangeHandler = function(handle) { this.ChangeHandler = handle; };
			this.isGroupAdded = function(id) {
				return (this.Groups.getIdxByProperty("ID", id) > -1) ? true : false;
			};
			this.addGroup = function(id, parentID, name) {
				if(!this.isGroupAdded(id)) {
					this.Groups.add({ID: id, ParentID: parentID, Name: name});
					if(this.ChangeHandler != null)
						this.ChangeHandler();
				}
			};
			this.removeGroup = function(id) {
				var idx = this.Groups.getIdxByProperty("ID", id);
				if(idx > -1) {
					this.Groups.remove(idx);
					if(this.ChangeHandler != null)
						this.ChangeHandler();
				}
			};
			this.resetGroups = function() {
				this.Groups.flush();
				if(this.ChangeHandler != null)
					this.ChangeHandler();
			};
			this.createQuery = function() {
				var query = "SELECT GOODS BY";
				if(!isEmpty(this.Barcode))
					query += " CODE(" + this.Barcode + ")";
				if(!isEmpty(this.Name))
					query += " SUBNAME(" + this.Name + ")";
				if(!isEmpty(this.Groups)) {
					var len = this.Groups.getCount();
					for(var i = 0; i < len; i++)
						query += " PARENT.ID(" + this.Groups.at(i).ID + ")";
				}
				if(this.BrandID > 0)
					query += " BRAND.ID(" + this.BrandID + ")";
				if(this.ManufID > 0)
					query += " MANUF.ID(" + this.ManufID + ")";
				if(!isEmpty(this.Template))
					query += " FORMAT.TDDO(" + this.Template + ")";
				return query;
			};
		},
		Util : {
			/* Проверка фильтра
			 *    Коды возврата
			 * 0    - OK
			 * 1001 - Не указаны критерии
			 * 1002 - Длина подстроки наименования меньше 4 символов
			 * 1003 - Строка штрихкода содержит некорректные символы
			 * 1004 - Длина подстроки штрихкода меньше 5 символов
			 * 1005 - Не указан шаблон вывода
			 */
			checkFilter : function(F) {
				var r = 0;
				var ok = false;
				if(!isEmpty(F.Name)) {
					ok = true;
					if(F.Name.length < 4)
						r = 1002;
				}
				if(!isEmpty(F.Barcode)) {
					ok = true;
					var ch;
					var len = F.Barcode.length;
					for(var i = 0; i < len; i++) {
						ch = F.Barcode.charAt(i);
						if(isNaN(ch)) {
							if(i == 0 && ch == '*') {
								if(len < 6) {
									r = 1004;
									break;
								}
							}
							else {
								r = 1003;
								break;
							}
						}
					}
				}
				if(F.Groups.getCount() > 0)
					ok = true;
				if(F.BrandID > 0)
					ok = true;
				if(F.ManufID > 0)
					ok = true;
				if(isEmpty(F.Template))
					r = 1005;
				if(!ok)
					r = 1001;
				return r;
			}
		},
		/* Функции */
		/* Создание/изменение товара */
		create : function(goods) {
			var id = 0;
			if(goods instanceof UHTT.Goods.Rec) {
				var url = "/dispatcher/goods/create";
				jQuery.ajax({
					type: "POST",
					url: url,
					async: false,
					data: UHTT.Util.JSON.toString(goods),
					contentType: "application/json; charset=utf-8",
				    dataType: "json",
					success: function(data, status, hdr) {
						id = parseInt(data);
					},
					error: AJAX_ERR_HANDLER
				}).done(function() {
				});
			}
			return id;
		},
		/* Выборка товаров по фильтру */
		fetch : function(F) {
			var data = null;
			if(!isEmpty(F)) {
				var query = F.createQuery();
				if(!isEmpty(query))
					data = UHTT.requestData(null, query);
			}
			return data;
		},
		/* Получение структуры товара по идентификатору */
		search : function(id) {
			var goods = null;
			var url = "/dispatcher/goods/get?id=" + id;
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: function(data, status, hdr) 
					{
						goods = new UHTT.Goods.Rec(data);
					},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() 
				{
				}
			);
			return goods;
		}
	},
	Brand : {
		Rec : function(json) {
			this.ID = 0;
			this.OwnerID = 0;
			this.GoodsGrpID = 0;
			this.Name = "";
			this.Code = "";
			if(!isEmpty(json)) {
				var _obj = UHTT.Util.JSON.parse(json);
				if(_obj != null) {
					this.ID = _obj.ID;
					this.OwnerID = _obj.OwnerID;
					this.GoodsGrpID = _obj.GoodsGrpID;
					this.Name = _obj.Name.unescapeHtml();
					this.Code = _obj.Code.unescapeHtml();
				}
			}
		},
		Filter : function(template) {
			/* ----------------------------------------------------------------- */
			/* Критерии */
			this.ID = 0;
			this.Name = "";
			this.OwnerID = 0;
			/* Шаблон вывода данных */
			this.Template = isEmpty(template) ? "VIEW_BRAND" : template;
			/* Обработчик события изменения фильтра */
			this.ChangeHandler = null;
			/* ----------------------------------------------------------------- */
			/* Функции */
			this.setID = function(id) { this.ID = id; };
			this.setName = function(name) { this.Name = name; };
			this.setOwnerID = function(id) { this.OwnerID = id; };
			this.createQuery = function() {
				var query = "SELECT BRAND BY";
				if(this.ID > 0)
					query += " ID(" + this.ID + ")";
				if(!isEmpty(this.Name))
					query += " SUBNAME(" + this.Name + ")";
				if(this.OwnerID > 0)
					query += " OWNER(" + this.OwnerID + ")";
				if(!isEmpty(this.Template))
					query += " FORMAT.TDDO(" + this.Template + ")";
				return query;
			};
		},
		Util : {
			//
			// Проверка фильтра
			//    Коды возврата
			//       0    - OK
			//       1001 - Не указан шаблон вывода
			//
			checkFilter : function(F) {
				var r = 0;
				if(isEmpty(F.Template))
					r = 1001;
				return r;
			}
		},
		//
		// Создание/изменение бренда
		//
		create : function(brand) {
			var id = 0;
			if(brand instanceof UHTT.Brand.Rec) {
				brand.Name = brand.Name.escapeHtml();
				brand.Code = brand.Code.escapeHtml();
				var reply = UHTT.requestData(null, "SET DL600 Brand " + UHTT.Util.JSON.toString(brand));
				if(!isEmpty(reply) && !isNaN(reply))
					id = parseInt(reply);
			}
			return id;
		},
		//
		// Выборка брендов по фильтру 
		//
		fetch : function(F) {
			var data = null;
			if(!isEmpty(F)) {
				var query = F.createQuery();
				if(!isEmpty(query))
					data = UHTT.requestData(null, query);
			}
			return data;
		},
		//
		// Получение структуры бренда по идентификатору 
		//
		getBrandByID : function(id) {
			var rec = null;
			var data = UHTT.requestData(null, "GETTDDO DATA_BRAND " + id);
			if(!isEmpty(data))
				rec = new UHTT.Brand.Rec(data);
			return rec;
		},
		//
		// Получение наименования бренда по идентификатору 
		//
		getNameByID : function(id) {
			var F = new UHTT.Brand.Filter("_TXT");
			F.ID = id;
			return this.fetch(F);
		}
	},
	// 
	// TODO: Person mark
	Person : {
		Address : {
			Rec : function(_obj) {
				this.LocID = 0;
				this.CityID = 0;
				this.LocKind = 0;
				this.LocCode = "";
				this.LocName = "";
				this.ZIP = "";
				this.Address = "";
				this.City = "";
				this.Country = "";
				this.OwnerID = 0;
				this.Longitude = 0.0;
				this.Latitude = 0.0;
				if(!isNull(_obj)) {
					if("LocID" in _obj)
						this.LocID = _obj.LocID;
					if("CityID" in _obj)
						this.CityID = _obj.CityID;
					if("LocKind" in _obj)
						this.LocKind = _obj.LocKind;
					if("LocCode" in _obj)
						this.LocCode = _obj.LocCode.unescapeHtml();
					if("LocName" in _obj)
						this.LocName = _obj.LocName.unescapeHtml();
					if("ZIP" in _obj)
						this.ZIP = _obj.ZIP.unescapeHtml();
					if("Address" in _obj)
						this.Address = _obj.Address.unescapeHtml();
					if("City" in _obj)
						this.City = _obj.City.unescapeHtml();
					if("Country" in _obj)
						this.Country = _obj.Country.unescapeHtml();
					if("OwnerID" in _obj)
						this.OwnerID = _obj.OwnerID;
					if("Longitude" in _obj)
						this.Longitude = _obj.Longitude;
					if("Latitude" in _obj)
						this.Latitude = _obj.Latitude;
				}
			},
			Editor : _extends(BaseEditor, {
				edit : function(kind, idx) {
					if(isNull(UHTT.Person.Editor.Object))
						throw new Error("Error: Person editor not init");
					this.Object = null;
					if(idx > -1) {
						if(!isEmpty(UHTT.Person.Editor.Object.AddrList) && (idx < UHTT.Person.Editor.Object.AddrList.length))
							this.Object = new UHTT.Person.Address.Rec(UHTT.Person.Editor.Object.AddrList[idx]);
						if(isNull(this.Object))
							idx = -1;
					}
					this.IsEditing = !isNull(this.Object);
					if(!this.IsEditing)
						this.Object = new UHTT.Person.Address.Rec({ LocKind: kind });
					this.Args.flush();
					this.Args.add({ Idx: idx });
					new SDialog("UHTT_PERSON_ADDRESS_EDITOR_FRM", UHTT.Content.get("PERSON_ADDRESS_EDITOR", {}),
							{ modal: true, position: "center" }, UHTT.Dialog.TYPE.INLINE);
				}
			})
		},
		Rec : function(json) {
			this.ID = 0;
			this.Name = "";
			this.StatusID = 0;
			this.CategoryID = 0;
			this.KindList = [];
			this.RegisterList = [];
			this.AddrList = [];
			this.PhoneList = []; // @v8.9.12
			this.EMailList = []; // @v8.9.12
			if(!isEmpty(json)) {
				var _obj = UHTT.Util.JSON.parse(json);
				if(_obj != null) {
					this.ID = _obj.ID;
					this.Name = _obj.Name.unescapeHtml();
					this.StatusID = _obj.StatusID;
					this.CategoryID = _obj.CategoryID;
					if(!isEmpty(_obj.KindList)) {
						for(var i = 0, len = _obj.KindList.length; i < len; i++) {
							var kind = _obj.KindList[i];
							if(!isEmpty(kind))
								this.KindList.push(kind);
						}
					}
					if(!isEmpty(_obj.RegisterList)) {
						for(var i = 0, len = _obj.RegisterList.length; i < len; i++) {
							var reg = _obj.RegisterList[i];
							if(!isEmpty(reg)) {
								reg.RegNumber = reg.RegNumber.unescapeHtml();
								this.RegisterList.push(reg);
							}
						}
					}
					if(!isEmpty(_obj.AddrList)) {
						for(var i = 0, len = _obj.AddrList.length; i < len; i++) {
							var addr = _obj.AddrList[i];
							if(!isEmpty(addr)) {
								addr.LocCode = addr.LocCode.unescapeHtml();
								addr.LocName = addr.LocName.unescapeHtml();
								addr.ZIP = addr.ZIP.unescapeHtml();
								addr.Address = addr.Address.unescapeHtml();
								this.AddrList.push(addr);
							}
						}
					}
					// @v8.9.12 {
					if(!isEmpty(_obj.PhoneList)) {
						for(var i = 0, len = _obj.PhoneList.length; i < len; i++) {
							var phone_item = _obj.PhoneList[i];
							if(!isEmpty(phone_item)) {
								phone_item.Code = phone_item.Code.unescapeHtml();
								this.PhoneList.push(phone_item);
							}
						}					
					}
					if(!isEmpty(_obj.EMailList)) {
						for(var i = 0, len = _obj.EMailList.length; i < len; i++) {
							var eml_item = _obj.EMailList[i];
							if(!isEmpty(eml_item)) {
								eml_item.Code = eml_item.Code.unescapeHtml();
								this.EMailList.push(eml_item);
							}
						}										
					}			
					// } @v8.9.12 		
				}
			}
		},
		Filter : function(kind, template) 
		{
			//
			// Критерии
			//
			this.Kind = kind;
			this.Name = "";
			this.StatusID = 0;
			this.CategoryID = 0;
			this.RegisterCode = "";
			this.RegisterNumber = "";
			//
			// Шаблон вывода данных 
			//
			this.Template = isEmpty(template) ? "VIEW_PERSON" : template;
			//
			// Обработчик события изменения фильтра 
			//
			this.ChangeHandler = null;
			//
			// Функции
			//
			this.setName = function(name) { this.Name = name; };
			this.setKind = function(kind) { this.Kind = kind; };
			this.setStatusID = function(id) { this.StatusID = id; };
			this.setCategoryID = function(id) { this.CategoryID = id; };
			this.setRegister = function(code, number) { this.RegisterCode = code; this.RegisterNumber = number; };
			this.setTemplate = function(template) { this.Template = template; };
			this.setChangeHandler = function(handle) { this.ChangeHandler = handle; };
			this.createQuery = function() 
			{
				var query = "SELECT PERSON BY";
				if(!isEmpty(this.Kind))
					query += " KIND.CODE(" + this.Kind + ")";
				if(!isEmpty(this.Name))
					query += " SUBNAME(" + this.Name + ")";
				if(this.StatusID > 0)
					query += " STATUS(" + this.StatusID + ")";
				if(this.CategoryID > 0)
					query += " CATEGORY(" + this.CategoryID + ")";
				if(!isEmpty(this.RegisterCode) && !isEmpty(this.RegisterNumber))
					query += " REGISTER.CODE(" + this.RegisterCode + ", " + this.RegisterNumber + ")";
				if(!isEmpty(this.Template))
					query += " FORMAT.TDDO(" + this.Template + ")";
				return query;
			};
		},
		Editor : _extends(BaseEditor, 
			{
				setAddress : function(idx, addr) {
					var ok = true;
					if(!(addr instanceof UHTT.Person.Address.Rec))
						throw new Error("Error: Invalid object type");
					if(!isNull(this.Object) && !isNull(this.Object.AddrList)) {
						for(var i = 0, n = this.Object.AddrList.length; i < n; i++) {
							var item = this.Object.AddrList[i];
							if(!isNull(item)) {
								if((!isEmpty(item.Code)) && (item.Code === addr.Code)) {
									if(idx != i) {
										// TODO: Error: Duplicate address code(symb)
										ok = false;
										break;
									}
								}
							}
						}
						if(ok) {
							if(idx < 0)  // создание
								this.Object.AddrList.push(addr);
							else {  // изменение
								if(idx < this.Object.AddrList.length)
									this.Object.AddrList[idx] = addr;
							}
							this.processEvent();
						}
					}
					return ok;
				},
				removeAddr : function(idx) 
				{
					if(!isNull(this.Object) && !isNull(this.Object.AddrList)) {
						if((idx > -1) && (idx < this.Object.AddrList.length)) {
							this.Object.AddrList.splice(idx, 1);
							this.processEvent();
						}
					}
				},
				edit : function(kindID, id) 
				{
					if(kindID == 0)
						throw new Error("Person editor init error: Invalid person kind ID");
					this.Object = null;
					if(id > 0)
						this.Object = UHTT.Person.getPersonByID(id);
					this.IsEditing = !isNull(this.Object);
					if(!this.IsEditing)
						this.Object = new UHTT.Person.Rec();
					if(isEmpty(this.Object.KindList))
						this.Object.KindList[0] = { KindID: kindID };
					new SDialog("UHTT_PERSON_EDITOR_FRM", UHTT.Content.get("PERSON_EDITOR", {}), 
						{ modal: true, position: "center" }, UHTT.Dialog.TYPE.INLINE);
				}
			}
		),
		Util : {
			//
			// Проверка фильтра
			//    Коды возврата
			//       0    - OK
			//       1001 - Не указаны критерии
			//       1002 - Не указан вид персоналии
			//       1003 - Длина подстроки наименования меньше 4 символов
			//       1004 - Не указан шаблон вывода
			//
			checkFilter : function(F) 
			{
				var r = 0;
				var ok = false;
				if(!isEmpty(F.Name)) {
					ok = true;
					if(F.Name.length < 4)
						r = 1003;
				}
				if(F.StatusID > 0)
					ok = true;
				if(F.CategoryID > 0)
					ok = true;
				if(isEmpty(F.Kind))
					r = 1002;
				if(isEmpty(F.Template))
					r = 1004;
				if(!ok)
					r = 1001;
				return r;
			}
		},
		//
		// Выборка персоналий по фильтру
		//
		fetch : function(F) 
		{
			var data = null;
			if(!isEmpty(F)) {
				var query = F.createQuery();
				if(!isEmpty(query))
					data = UHTT.requestData(null, query);
			}
			return data;
		},
		//
		// Получение структуры персоналии по идентификатору
		//
		getPersonByID : function(id) 
		{
			var person = null;
			var url = "/dispatcher/person/get?id=" + id;
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: function(data, status, hdr) 
					{
						person = new UHTT.Person.Rec(data);
					},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() 
				{
				}
			);
			return person;
		},
		create : function(person) 
		{
			var id = 0;
			if(!isNull(person)) {
				var url = "/dispatcher/person/create";
				jQuery.ajax(
					{
						type: "POST",
						url: url,
						async: false,
						data: UHTT.Util.JSON.toString(person),
						contentType: "application/json; charset=utf-8",
						dataType: "json",
						success: function(data, status, hdr) 
						{
							id = parseInt(data);
						},
						error: AJAX_ERR_HANDLER
					}
				).done(
					function() {
					}
				);
			}
			return id;
		}
	},
	SpecialSeries : {
		Filter : function(template) {
			/* ----------------------------------------------------------------- */
			/* Критерии */
			this.Period = "";
			this.Serial = "";
			this.GoodsGroupID = 0;
			this.GoodsID = 0;
			/* Шаблон вывода данных */
			this.Template = isEmpty(template) ? "VIEW_SPECSERIES" : template;
			/* Обработчик события изменения фильтра */
			this.ChangeHandler = null;
			/* ----------------------------------------------------------------- */
			/* Функции */
			this.setPeriod = function(period) { this.Period = period; };
			this.setSerial = function(serial) { this.Serial = serial; };
			this.setGoodsGroupID = function(id) { this.GoodsGroupID = id; };
			this.setGoodsID = function(id) { this.GoodsID = id; };
			this.setTemplate = function(template) { this.Template = template; };
			this.setChangeHandler = function(handle) { this.ChangeHandler = handle; };
			this.createQuery = function() {
				var query = "SELECT SPECSERIES BY";
				if(!isEmpty(this.Period))
					query += " PERIOD(" + this.Period + ")";
				if(!isEmpty(this.Serial))
					query += " CODE(" + this.Serial + ")";
				if(this.GoodsID > 0)
					query += " GOODS(" + this.GoodsID + ")";
				if(!isEmpty(this.Template))
					query += " FORMAT.TDDO(" + this.Template + ")";
				return query;
			};
		},
		/* Функции */
		/* Выборка специальных серий по фильтру */
		fetch : function(F) {
			var data = null;
			if(!isEmpty(F)) {
				var query = F.createQuery();
				if(!isEmpty(query))
					data = UHTT.requestData(null, query);
			}
			return data;
		}
	},
	SCard : {
		Filter : function(template) {
			/* ----------------------------------------------------------------- */
			/* Критерии */
			this.SeriesID = 0;
			this.Number = "";
			/* Шаблон вывода данных */
			this.Template = isEmpty(template) ? "VIEW_SCARD" : template;
			/* Обработчик события изменения фильтра */
			this.ChangeHandler = null;
			/* ----------------------------------------------------------------- */
			/* Функции */
			this.Number = function(number) { this.Number = number; };
			this.setTemplate = function(template) { this.Template = template; };
			this.setChangeHandler = function(handle) { this.ChangeHandler = handle; };
			this.createQuery = function() {
				var query = "SELECT SCARD BY";
				if(this.SeriesID > 0)
					query += " SERIES.ID(" + this.SeriesID + ")";
				if(!isEmpty(this.Number))
					query += " CODE(" + this.Number + ")";
				if(!isEmpty(this.Template))
					query += " FORMAT.TDDO(" + this.Template + ")";
				return query;
			};
		}
	},
	Workbook : {
		Rec : function(json) {
			this.ID = 0;
			this.ParentID = 0;
			this.CssID = 0;
			this.LinkID = 0;
			this.Name = "";
			this.Symb = "";
			this.Type = 0;
			this.Flags = 0;
			this.Dt = "";
			this.Tm = "";
			this.Descr = "";
			//public Tag    TagList[];
			if(!isEmpty(json)) {
				var _obj = UHTT.Util.JSON.parse(json);
				if(_obj != null) {
					this.ID = _obj.ID;
					this.ParentID = _obj.ParentID;
					this.CssID = _obj.CssID;
					this.LinkID = _obj.LinkID;
					this.Name = _obj.Name;
					this.Symb = _obj.Symb;
					this.Type = _obj.Type;
					this.Flags = _obj.Flags;
					this.Dt = _obj.Dt;
					this.Tm = _obj.Tm;
					this.Descr = _obj.Descr;
				}
			}
		},
		Filter : function() {
			//
			// Критерии
			//
			this.ID = 0;
			this.ParentID = 0;
			this.Type = 0;
			this.Code = "";
			this.Name = "";
			// Обработчик события изменения фильтра
			this.ChangeHandler = null;
			//
			// Функции
			//
			this.setChangeHandler = function(handle) 
			{ 
				this.ChangeHandler = handle; 
			};
			this.getValues = function() 
			{
				var list = new SList();
				if(this.ID > 0)
					list.add(new SPair("ID", this.ID));
				if(this.ParentID > 0)
					list.add(new SPair("ParentID", this.ParentID));
				if(this.Type > 0)
					list.add(new SPair("Type", this.Type));
				if(!isEmpty(this.Code))
					list.add(new SPair("Code", this.Code));
				if(!isEmpty(this.Name))
					list.add(new SPair("Name", this.Name));
				return list.getArray();
			};
		},
		// Функции 
		// Выборка по фильтру 
		fetch : function(F) 
		{
			UHTT.BlockUI.block();
			var _data = null;
			var url = new SUrl("/dispatcher/workbook/get");
			url.Parameters.setArray(F.getValues());
			jQuery.ajax(
				{
					url: url.toString(),
					async: false,
					success: 
						function(data, status, hdr) 
						{
							try {
								_data = UHTT.Util.JSON.parse(data);
							} catch(e) 
							{
							}
						},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() 
				{
					UHTT.BlockUI.unblock();
				}
			);
			return _data;
		},
		//
		// Получение контента
		//
		getContent : function(ident, params) 
		{
			UHTT.BlockUI.block();
			var _data = null;
			var url = new SUrl("/dispatcher/workbook/content");
			if(isInteger(ident)) {
				url.Parameters.add(new SPair("id", ident));
			}
			else if(isString(ident)) {
				url.Parameters.add(new SPair("code", ident));
			}
			else {
				throw new Error("invalid parameter data type");
			}
			if(!isEmpty(params)) {
				for(var prop in params) {
					if(!isEmpty(prop)) {
						var val = params[prop];
						if(!isEmpty(val))
							url.Parameters.add(new SPair(prop, val));
					}
				}
			}
			jQuery.ajax(
				{
					url: url.toString(),
					async: false,
					success: 
						function(data, status, hdr) 
						{
							try {
								_data = data;
							} catch(e) 
							{
							}
						},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() 
				{
					UHTT.BlockUI.unblock();
				}
			);
			return _data;
		}
	},
	Content: {
		get : function(name, args) 
		{
			UHTT.BlockUI.block();
			var content = "";
			var url = '/dispatcher/content?name=' + encodeURIComponent(name) + 
				(!isEmpty(args) ? "&args=" + encodeURIComponent(UHTT.Util.JSON.toString(args)) : "");
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: 
						function(data, status, hdr) 
						{
							content = data;
						},
					error: 
						function(request, status, error) 
						{
							UHTT.Messenger.show("Ошибка: " + error);
						}
				}
			).done(
				function() 
				{
					UHTT.BlockUI.unblock();
				}
			);
			return content;
		},
		getTddt : function(label) 
		{
			UHTT.BlockUI.block();
			var url = '/dispatcher/tddt?label=' + encodeURIComponent(label);
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: function(data, status, hdr) 
					{
						content = data;
					},
					error: function(request, status, error) 
					{
						UHTT.Messenger.show("Ошибка: " + error);
					}
				}
			).done(
				function() {
					UHTT.BlockUI.unblock();
				}
			);
			return content;
		}
	},
	PPObjID : function() {
		this.Obj = 0;
		this.Id = 0;
	},
	PPObjIDNames : function() {
		this.Obj = 0;
		this.Id = 0;
		this.Name = "";
	},	
	hasNaturalTokenType : function(nt, type)
	{
		var result = 0.0;
		if(!isNull(nt) && isArray(nt.TypeList)) {
			var tc = nt.TypeList.length;
			if(isString(type)) {
				for(var i = 0; i < tc; i++) {
					var t = nt.TypeList[i];
					if(t.Symb == type) {
						result = t.Prob;
						break;
					}
				}
			}
			else if(isInteger(type)) {
				for(var i = 0; i < tc; i++) {
					var t = nt.TypeList[i];
					if(t.Id == type) {
						result = t.Prob;
						break;
					}
				}			
			}
		}
		return result;
	},
	NaturalToken : {
		Type : function() 
		{
			this.Id = 0;
			this.Symb = "";
			this.Prob = 0.0;
		},
		Rec : function(json) 
		{
			this.Token = "";
			this.TypeList = [];
			this.RelObjList = [];
			if(!isEmpty(json)) {
				var _obj = UHTT.Util.JSON.parse(json);
				if(_obj != null) {
					this.Token = _obj.Token;
					this.TypeList = _obj.TypeList;
					this.RelObjList = _obj.RelObjList;
				}
			}			
		},
		get : function(ntoken) 
		{
			var    ntok_result = 0;
			if(!isEmpty(ntoken)) {
				var url = "/dispatcher/querynattoken?ntok=" + encodeURIComponent(UHTT.Util.Base64.encode(ntoken));
				jQuery.ajax(
					{
						url: url,
						async: false,
						success: 
							function(data, status, hdr) 
							{
								ntok_result = UHTT.Util.JSON.parse(data);
							},
						error: AJAX_ERR_HANDLER
					}
				).done(
					function() 
					{
					}
				);
			}
			return ntok_result;		
		}
	},
	ObjectUtil : {
		getStrAssocAry : function(objType, criter) {
			var ary = null;
			var url = "/dispatcher/get_sa_ary?obj_type=" + objType + "&criter=" + UHTT.Util.JSON.toString(criter);
			jQuery.ajax({
				url: url,
				async: false,
				success: function(data, status, hdr) {
					ary = UHTT.Util.JSON.parse(data);
				},
				error: AJAX_ERR_HANDLER
			}).done(function() {
			});
			return ary;
		},
		getStrAssocItem : function(objType, criter) {
			var item = null;
			var sa_ary = this.getStrAssocAry(objType, criter);
			if(!isEmpty(sa_ary)) {
				item = sa_ary[0];
			}
			return item;
		}
	},
	UserUtil : {
		UserChoiseForm : {
			/* Changes handler */
			Callback : null,
			/* --- */
			open : function(callback) {
				this.Callback = callback;
				new SDialog("UHTT_CHOISE_USER_FORM", "GETTDDO FRM_CHOISE_USER", { modal:true, position:"center" });
			},
			clear : function() {
				this.Callback = null;
			}
		},
		getCurrentUser : function() {
			var user = null;
			var url = "/dispatcher/get_current_user";
			jQuery.ajax({
				url: url,
				async: false,
				success: function(data, status, hdr) {
					if(!isEmpty(data))
						user = UHTT.Util.JSON.parse(data);
				},
				error: function(request, status, error) {
					UHTT.Messenger.show("Ошибка: " + error);
				}
			}).done(function() {
			});
			return user;
		},
		getUser : function(id) {
			var user = null;
			if(isInteger(id)) {
				var url = "/dispatcher/dc/get_users?id=" + id;
				jQuery.ajax({
					url: url,
					async: false,
					success: function(data, status, hdr) {
						user = UHTT.Util.JSON.parse(data);
					},
					error: function(request, status, error) {
						UHTT.Messenger.show("Ошибка: " + error);
					}
				}).done(function() {
				});
			}
			return user;
		},
		getUsers : function() {
			var list = null;
			var url = "/dispatcher/dc/get_users";
			jQuery.ajax({
				url: url,
				async: false,
				success: function(data, status, hdr) {
					list = new SList(UHTT.Util.JSON.parse(data));
				},
				error: function(request, status, error) {
					UHTT.Messenger.show("Ошибка: " + error);
				}
			}).done(function() {
			});
			return list;
		}
	},
	getNaturalToken : function(ntoken) 
	{
		var    ntok_result = 0;
		if(!isEmpty(ntoken)) {
			var url = "/dispatcher/querynattoken?ntok=" + encodeURIComponent(UHTT.Util.Base64.encode(ntoken));
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: 
						function(data, status, hdr) 
						{
							ntok_result = UHTT.Util.JSON.parse(data);
						},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() 
				{
				}
			);
		}
		return ntok_result;		
	},	
	//
	// Request
	//
	requestData : function(receiver, query, noblk) 
	{
		if(noblk == true) {
		}
		else {
			UHTT.BlockUI.block();
		}
		var reply = "";
		var url = "/dispatcher/?query=" + encodeURIComponent(query);
		jQuery.ajax({
			url: url,
			async: false,
			success: function(data, status, hdr) 
			{
				var msg = hdr.getResponseHeader("X-UHTT-Message");
				if(!isEmpty(msg))
					UHTT.Messenger.show(msg);
				if(isEmpty(receiver))
					reply = data;
				else if(!isEmpty(data))
					UHTT.Event.fire(receiver, data);
			},
			error: function(request, status, error) {
				UHTT.Messenger.show("Ошибка: " + error);
			}
		}).done(function() {
			if(noblk == true) {
			}
			else {
				UHTT.BlockUI.unblock();
			}
		});
		return reply;
	},
	askQuestion : function(email, question) 
	{
		var    ok = 0;
		if(!isEmpty(email) && !isEmpty(question)) {
			var url = "/dispatcher/ask_question?email=" + encodeURIComponent(UHTT.Util.Base64.encode(email)) + 
				"&question=" + encodeURIComponent(UHTT.Util.Base64.encode(question));
			jQuery.ajax(
				{
					url: url,
					async: false,
					success: 
						function(data, status, hdr) 
						{
							ok = 1;
						},
					error: AJAX_ERR_HANDLER
				}
			).done(
				function() 
				{
				}
			);
		}
		return ok;
	},
	init : function() 
	{
		UHTT.BlockUI.setup("Загрузка...", "/rsrc/images/loader.gif");
	}
};
//
// PROFILER
//
var _PROFILER = {
	Block : function(id, name) {
		this.ID = id;
		this.Name = name;
		this.StartTime = new Date();
	},
	BlkCount : 0,
	Blocks : new SList(),
	Start : function(name) {
		var blk = new _PROFILER.Block(++this.BlkCount, name);
		this.Blocks.add(blk);
		return blk.ID;
	},
	End : function(name) {
		var _t = new Date();
		var idx = this.Blocks.getIdxByProperty("Name", name);
		var blk = this.Blocks.at(idx);
		if(!isEmpty(blk)) {
			console.log("PROFILER:: Block \"%s\" time: %d ms", blk.Name, _t - blk.StartTime);
			this.Blocks.remove(idx);
		}
		else {
			console.log("PROFILER:: Unknown block name");
		}
	}
};
//
//
//
var SkinnyTip = SkinnyTip || {
	divId: 'skinnytip-layer',
	mouseX: null,
	mouseY: null,
	zIndexLayer: 10000,
	text: null,
	title: null,
	xOffset:15,
	yOffset: 15
};
SkinnyTip.reset = function() 
{
	this.xOffset = 15;
	this.yOffset = 15;
	this.backColor = '#FFC';
	this.borderColor = '#FC6';
	this.textColor = '#000';
	this.titleTextColor = '#000';
	this.width = '300px';
	this.border = '2px';
	this.titlePadding = '1px';
	this.textPadding = '1px 3px';
	this.fontFace = 'Arial, Helvetica, Sans-Serif';
	this.fontSize = '14px';
	this.titleFontSize = '14px';
	this.layer = null;
	this.zIndex = 0;
	this.visible = false;
};
SkinnyTip.init = function() 
{
	var targets = document.querySelectorAll(".skinnytip");
	var targetCount = targets.length;
	for (var i = 0; i < targetCount; i++) {
		targets[i].addEventListener('mouseover', function() {
			var title, text, options;
			if(this.hasAttribute('data-title')) {
				title = this.getAttribute('data-title');
			}
			if(this.hasAttribute('data-text')) {
				text = this.getAttribute('data-text');
			}
			if(this.hasAttribute('data-options')) {
				options = this.getAttribute('data-options');
			}
			SkinnyTip.tooltip(text, title, options);
		});
		targets[i].addEventListener('mouseout', function() {
			SkinnyTip.hideTip();
		});
	}
	targets = null;
	this.captureMouse();
};

SkinnyTip.tooltip = function(text, title, options) 
{
	if(!text & !title) {
		return false;
	}
	//Reset variables for this tool tip
	this.reset();
	this.title = title;
	this.text = text;
	if(!(this.layer = self.document.getElementById(this.divId))) {
		var div = document.createElement("div");
		div.id = this.divId;
		div.style.visibility = "hidden";
		div.style['z-index'] = "10000";
		div.style.position = "absolute";
		document.body.appendChild(div);
		this.layer = div;
	}
	//if we have mouse coords, create and show tooltip
	if(this.mouseX && this.mouseY) {
		this.parseOptions(options);
		this.assemble(this.getMarkup(this.text, this.title));
		this.position();
		this.layer.style.visibility = 'visible';
		this.visible = true;
	}
};

// Set mouse handler callback.
SkinnyTip.captureMouse = function() 
{
	var self = this;
	document.onmousemove = SkinnyTip.mouseMoveHandler.bind(this);
};

// Callback for document.onmousemove
SkinnyTip.mouseMoveHandler = function(e) 
{
	if(!e)
		e = event;
	// if there is an x pos property, get mouse location
	this.mouseX = this.getMouseX(e);
	this.mouseY = this.getMouseY(e);
	if(this.visible)
		this.position();
};
//
// get mouse x coords
//
SkinnyTip.getMouseX = function(e) 
{
	if(e.pageX)
		return e.pageX;
	else
		return e.clientX ? e.clientX + 
			(document.documentElement.scrollLeft ? document.documentElement.scrollLeft : document.body.scrollLeft) : this.mouseX;
};

//get mouse y coords
SkinnyTip.getMouseY = function(e) 
{
	if(e.pageY) { 
		return e.pageY; 
	}
	return e.clientY ? e.clientY + 
		(document.documentElement.scrollTop ? document.documentElement.scrollTop : document.body.scrollTop) : this.mouseY;
};

SkinnyTip.parseOptions = function(options) 
{
	if(options) {
		var optArr = options.split(',');
		for(var i = 0; i < optArr.length; i++) {
			var args = optArr[i].split(':');
			eval('this.' + this.trimWhitespace(args[0]) + '="' + this.trimWhitespace(args[1]) + '"');
		}
	}
};

SkinnyTip.hideTip = function() 
{
	if(this.visible && this.layer) {
		this.layer.style.visibility = 'hidden';
		this.visible = false;
	}
};

SkinnyTip.getMarkup = function(text, title) 
{
	var containerStyle = 'width:' + this.width + ';' +
		'border:' + this.border + ' solid ' + this.borderColor + ';' +
		'background-color:' + this.backColor + ';' +
		'font-family:' + this.fontFace + ';' +
		'font-size:' + this.fontSize + ';';
	var titleStyle = 'background-color:' + this.borderColor + ';' +
		'padding:' + this.titlePadding + ';' +
		'color:' + this.titleTextColor + ';' +
		'font-size:' + this.titleFontSize + ';';
	var contentStyle = 'padding:' + this.textPadding + ';' + 'color:' + this.textColor + ';';
	var txt = '<div id="skinnytip-container" style="' + containerStyle + '">';
	if(title) {
		txt += '<div id="skinnytip-title" style="' + titleStyle + '">' + title + '</div>';
	}
	if(text) {
		txt += '<div id="skinnytip-content" style="' + contentStyle + '">' + text + '</div>';
	}
	txt += '</div>';
	return txt;
};
//
// Positions popup according to mouse input
//
SkinnyTip.position = function() 
{
	this.layer.style.left = this.getXPlacement() + 'px';
	this.layer.style.top = this.getYPlacement() + 'px';
};
//
//get horizontal box placement
//
SkinnyTip.getXPlacement = function() 
{
	return this.mouseX + parseInt(this.xOffset);
};
//
// get vertical box placement
//
SkinnyTip.getYPlacement = function() 
{
	return this.mouseY + parseInt(this.yOffset);
};
//
// Creates the popup
//
SkinnyTip.assemble = function(input) 
{
	if(typeof this.layer.innerHTML != 'undefined') {
		this.layer.innerHTML = '<div style="position: absolute; top: 0; left: 0; width: ' + 
			this.width + '; z-index: ' + (this.zIndex+1) + ';">' + input + '</div>';
	}
};

SkinnyTip.trimWhitespace = function(str) 
{
	return str.replace(/^\s+|\s+$/gm, '');
};
//
// Credit to Douglas Crockford for this
//
if(!Function.prototype.bind) {
	Function.prototype.bind = function (oThis) 
	{
		if(typeof this !== "function") {
			// closest thing possible to the ECMAScript 5 internal IsCallable functionâ€‹
			throw new TypeError ("Function.prototype.bind - is not callable");
		}
		var aArgs = Array.prototype.slice.call (arguments, 1),
			fToBind = this,
			fNOP = function() 
			{
			},
			fBound = function () 
			{
				return fToBind.apply (this instanceof fNOP && oThis ? this : oThis, aArgs.concat (Array.prototype.slice.call(arguments)));
			};
		fNOP.prototype = this.prototype;
		fBound.prototype = new fNOP ();
		return fBound;
	};
}
