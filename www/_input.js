var Input = Input || {};
Input.namespace = function(ns_string){
	var parts = ns_string.split('.'),
		parent = Input,
		i;
	if(parts[0] === "Input"){
		parts = parts.slice(1);
	}
	for(i = 0; i < parts.length; i += 1){
		if(typeof parent[parts[i]] === "undefined"){
			parent[parts[i]] = {};
		}
		parent = parent[parts[i]];
	}
	return parent;
};
/*****************************************************************************
 * ���O��Ԃ��`
 ****************************************************************************/
Input.namespace("Input.event");
Input.namespace("Input.Mouse");
Input.namespace("Input.holdkey");
Input.namespace("Input.Keybord");
/*****************************************************************************
 * Input.event
 * [�ʏ�̃C�x���g����]
 * element : �v�f
 * type    : �C�x���g�^�C�v
 *           abort,error,submit,reset,load,unload,blur,focus,change,select
 *           click,dblclick,mouseout,mouseover,mousedown,mouseup,mousemove,
 *           keypress,keyup,keydown,dragdrop,move,resize,scroll
 * callback: �C�x���g���Ύ��ɓ��삷��֐�
 *
 * event.add(element, type, callback);		event�o�^
 * event.remove(element, type, callback);	event�폜
 *
 * e       : �C�x���g
 * event.stop_default(e);					�f�t�H���g����̗}��
 * event.stop_bubble(e);					�o�u�����O�̗}��
 * event.stop_event(e);						�f�t�H���g����y�уo�u�����O�}��
 ****************************************************************************/
Input.event = (function() {
	//�ˑ��֌W
	var Handler = {};

	if (document.addEventListener) {
		Handler.add = function(el, type, callback, bubble) {
			el.addEventListener(type, callback, false);
		};
		Handler.remove = function(el, type, callback) {
			el.removeEventListener(type, callback, false);
		};
	} else {
		if (document.attachEvent) {
			Handler.add = function(el, type, callback) {
				if (Handler._find(el, type, callback) !== -1) {
					return;
				}
				var wrappedHandler = function(e) {
					e = e || window.event;
					var event = {
						_event: e,
						type: e.type,
						target: e.srcElement,
						currentTarget: el,
						relatedTarget: e.fromElement ? e.fromElement : e.toElement,
						eventPhase: (e.srcElement === el) ? 2 : 3,

						clientX: e.clientX,
						clientY: e.clientY,
						screenX: e.screenX,
						screenY: e.screenY,

						altKey: e.altKey,
						ctrlKey: e.ctrlKey,
						shiftKey: e.shiftKey,
						charCode: e.keyCode,

						stopPropagation: function() {
							this._event.cancelBubble = true;
						},
						preventDefault: function() {
							if (this._event && this._event.returnValue) {
								this._event.returnValue = false;
							}
						}
					};
					if (Function.prototype.call) {
						callback.call(el, event);
					} else {
						el._currentHandler = callback;
						el._currentHandler(event);
						el._currentHandler = null;
					}
				};
				el.attachEvent("on" + type, wrappedHandler);
				var h = {
						el: el,
						type: type,
						callback: callback,
						wrappedHandler: wrappedHandler
					},
					d = el.document || el,
					w = d.parentWindow,
					id = Handler._uid();

				if (!w._allHandlers) {
					w._allHandlers = {};
				}
				w._allHandlers[id] = h;

				if (!el._handlers) {
					el._handlers = [];
				}
				el._handlers.push(id);
				if (!w._onunloadHandlerRegistered) {
					w._onunloadHandlerRegistered = true;
					w.attachEvent("onunload", Handler._removeAllHandlers);
				}
			};
			Handler.remove = function(el, type, callback) {
				var i = Handler._find(el, type, callback);
				if (i === -1) {
					return;
				}

				var d = el.document || el,
					w = d.parentWindow,
					handlerId = el._handlers[i],
					h = w._allHandlers[handlerId];

				el.detachEvent("on" + type, h.wrappedHandler);
				el._handlers.splice(i, 1);

				delete w._allHandlers[handlerId];
			};
			Handler._find = function(el, type, callback) {
				var handlers = el._handlers;
				if (!handlers) {
					return -1;
				}
				var d = el.document || el,
					w = d.parentWindow,
					handlerId,
					h,
					i;

				for (i = handlers.length - 1; i >= 0; i -= 1) {
					handlerId = handlers[i];
					h = w._allHandlers[handlerId];
					if (h.type === type && h.callback === callback) {
						return i;
					}
				}
				return -1;
			};
			Handler._removeAllHandlers = function() {
				var w = this,
					uid,
					h;
				for (uid in w._allHandlers) {
					h = w._allHandlers[uid];
					h.el.detachEvent("on" + h.type, h.wrappedHandler);
					delete w._allHandlers[uid];
				}
			};
			Handler._counter = 0;
			Handler._uid = function() {
				return "h" + Handler._counter++;
			};
		}
	}
	Handler.stop_default = function(e) {
		if (e.preventDefault) {
			e.preventDefault();
		} else {
			e.returnValue = false;
		}
	};
	Handler.stop_bubble = function(e) {
		if (e.stopPropagation) {
			e.stopPropagation();
		} else {
			e.cancelBubble = true;
		}
	};
	Handler.stop_event = function(e) {
		Handler.stop_default(e);
		Handler.stop_bubble(e);
	};
	return Handler;
}());


/*****************************************************************************
 * Input.Mouse
 * [�}�E�X�C�x���g����]
 * element : canvas�v�f
 *
 * [�@�\]
 * update : �}�E�X�̏�Ԃ��X�V���鏈���i���[�v�����Ŗ���ĂԕK�v������j
 *
 * [update�㗘�p�ł���@�\]
 * get_click_position    : �N���b�N�����������ꏊ����Ԃ��B�i�������W�j
 *                         �N���b�N���Ă��Ȃ��ꍇ��false��Ԃ��B
 * get_dbclick_position  : �_�u���N���b�N�����������ꏊ������i�������W�j
 *                         �_�u���N���b�N���Ă��Ȃ��ꍇ��false��Ԃ��B
 * get_position          : �}�E�X�̏ꏊ������i�������W�j
 * get_wheel_delta       : �z�C�[���̕ω��ʁi1���j������B
 * get_button_states     : �}�E�X�̊e�{�^����������Ă��邩��Ԃ��B
 *
 *
 *[���ڎg�p����v���p�e�B]
 * absoluteX             : �}�E�X�̏ꏊ������i�v�f����������W�j
 * absoluteY             : �}�E�X�̏ꏊ������i�v�f����������W�j
 * relativeX             : �O��̃}�E�X�ʒu����̕ω��ʁi�v�f����������W�j
 * relativeY             : �O��̃}�E�X�ʒu����̕ω��ʁi�v�f����������W�j
 ****************************************************************************/
Input.Mouse = function(element){

	var instance,
		Mouse = Input.Mouse.prototype,
		Event = Input.event,
		i;

	if(!(this instanceof Input.Mouse)){
		return new Input.Mouse(element);
	}

	// singletron
	Input.Mouse = function(){
		return instance;
	};
	Input.Mouse.prototype = this;
	instance = new Input.Mouse(element);
	instance.constructor = Input.Mouse;

	this.element = element || document;
	this.element_width = this.element.width;
	this.element_height = this.element.height;
	this.element_half_width = this.element.width / 2;
	this.element_half_height = this.element.height / 2;

	this.curr_wheel_delta = 0;
	this.prev_wheel_delta = 0;
	this.wheel_delta = 0;

	this.position = null;
	this.absoluteX = null;
	this.absoluteY = null;
	this.relativeX = 0;
	this.relativeY = 0;

	this.weight_modifier = Mouse.WEIGHT_MODIFIER;
	this.history_buffer_size = Mouse.HISTORY_BUFFER_SIZE;

	this.filtered = [0.0, 0,0];
	this.enable_filtering = true;

	this.index = 0;
	this.movementX = [0, 0];
	this.movementY = [0, 0];

	this.button_states = [
							[0, 0, 0],	// current
							[0, 0, 0],	// prev
							[0, 0, 0]	// temp
						];
	this.curr_button_states = this.button_states[0];
	this.prev_button_states = this.button_states[1];
	this.temp_button_states = this.button_states[2];

	this.click_states = [
							[null, null],	// current
							[null, null]	// temp
						];
	this.curr_click = this.click_states[0];
	this.temp_click = this.click_states[1];

	this.dbclick_states = [
							[null, null],	// current
							[null, null]	// temp
						];
	this.curr_dbclick = this.dbclick_states[0];
	this.temp_dbclick = this.dbclick_states[1];

	this.history = [];
	for(i = 0; i < this.history_buffer_size; i += 1){
		this.history[i] = {
			x: 0,
			y: 0
		};
	}
	// add event
	Event.add(this.element, "click", function(e){Mouse.message.call(instance, e);});
	Event.add(this.element, "dblclick", function(e){Mouse.message.call(instance, e);});
	var ua = navigator.userAgent;
	if (ua.indexOf('iPhone') > -1 || ua.indexOf('iPad') > -1 || ua.indexOf('iPod') > -1) {
		Event.add(this.element, "touchup", function(e){Mouse.message.call(instance, e);});
		Event.add(this.element, "touchstart", function(e){Mouse.message.call(instance, e);});
		Event.add(this.element, "touchmove", function(e){Mouse.message.call(instance, e);});
	} else {
		Event.add(this.element, "mouseup", function(e){Mouse.message.call(instance, e);});
		Event.add(this.element, "mousedown", function(e){Mouse.message.call(instance, e);});
		Event.add(this.element, "mousemove", function(e){Mouse.message.call(instance, e);});
	}
	Event.add(this.element, "mouseout",  function(e){Mouse.message.call(instance, e);});
	Event.add(this.element, "mousewheel", function(e){Mouse.message.call(instance, e);});
	Event.add(this.element, "DOMMouseScroll", function(e){Mouse.message.call(instance, e);});
	return instance;
};

Input.Mouse.prototype =(function(){
	var BUTTON_LEFT   = 0,
		BUTTON_MIDDLE = 1,
		BUTTON_RIGHT  = 2,
		WEIGHT_MODIFIER = 0.4,
		HISTORY_BUFFER_SIZE = 10,

		last_message = new Date(),
		Event = Input.event,

		window_info = function(){
			var wininfo = {};

			if(window.innerWidth){
				wininfo.horizontal_scroll = window.pageXOffset;
				wininfo.vertical_scroll = window.pageYOffset;
			}else{
				if(document.documentElement && document.documentElement.clientWidth){
					wininfo.horizontal_scroll = document.documentElement.scrollLeft;
					wininfo.vertical_scroll = document.documentElement.scrollTop;
				}else{
					if(document.body.clientWidth){
						wininfo.horizontal_scroll = document.body.scrollLeft;
						wininfo.vertical_scroll = document.body.scrollTop;
					}
				}
			}
			return wininfo;
		},
		getX = function(el){
			var x = 0,
				e;
			for(e = el; e; e = e.offsetParent){
				x += e.offsetLeft;
			}
			for(e = el.parentNode; e && e !== document.body; e = e.parentNode){
				if(e.scrollLeft){
					x -= e.scrollLeft;
				}
			}
			return x;
		},
		getY = function(el){
			var y = 0,
				e;
			for(e = el; e; e = e.offsetParent){
				y += e.offsetTop;
			}
			for(e = el.parentNode; e && e !== document.body; e = e.parentNode){
				if(e.scrollTop){
					y -= e.scrollTop;
				}
			}
			return y;
		},

		get_position = function(e){
			var el = e.srcElement || e.target,
				wininfo = window_info();

			return {
				x: e.clientX + wininfo.horizontal_scroll - getX(el),
				y: e.clientY + wininfo.vertical_scroll - getY(el)
			};
		};

	return {
		BUTTON_LEFT: BUTTON_LEFT,
		BUTTON_RIGHT: BUTTON_RIGHT,
		BUTTON_MIDDLE: BUTTON_MIDDLE,

		WEIGHT_MODIFIER: WEIGHT_MODIFIER,
		HISTORY_BUFFER_SIZE: HISTORY_BUFFER_SIZE,

		convert_position: function(position){
			return {
				x: (position.x - this.element_half_width),
				y: (this.element_half_height - position.y)
			}
		},

		get_click_position: function(){
			if(this.curr_click[0] === null){
				return false;
			}
			return {
				x: this.curr_click[0],
				y: this.curr_click[1]
			};
		},
		get_dbclick_position: function(){
			if(this.curr_dbclick[0] === null){
				return false;
			}
			return {
				x: this.curr_dbclick[0],
				y: this.curr_dbclick[1]
			};
		},
		get_position: function(){
			return this.position;
		},
		get_button_states: function(button){
			//button = button || BUTTON_LEFT;
			//return this.curr_button_states[BUTTON_LEFT];
			return this.curr_button_states[button];
		},
		get_wheel_position: function(){
			return  this.wheel_delta;
		},
		get_wheel_pos: function(){
			return  this.wheel_delta;
		},
		message: function(event){
			var position = get_position(event),
				button = event.button,
				dx = 0, dy = 0,
				i;
			this.position = position;
			result = this.convert_position(position);
			if(this.absoluteX !== null){
				dx = result.x - this.absoluteX;
				dy = result.y - this.absoluteY;
			}

			if(this.enable_filtering){
				last_message = new Date();
				this.perform_mouse_filtering(dx, dy);
				this.relativeX = this.filtered[0];
				this.relativeY = this.filtered[1];

				this.perform_mouse_smoothing(this.relativeX, this.relativeY);
				this.relativeX = this.filtered[0];
				this.relativeY = this.filtered[1];
			}else{
				this.relativeX = dx;
				this.relativeY = dy;
			}
			switch(event.type){
				case "click":
					this.temp_click[0] =  position.x;
					this.temp_click[1] =  position.y;
					break;
				case "dblclick":
					this.temp_dbclick[0] =  position.x;
					this.temp_dbclick[1] =  position.y;
					break;
				case "mouseup":
					this.temp_button_states[button] = false;
					break;
				case "mousedown":
					this.temp_button_states[button] = true;
					break;
				case "mousemove":
					this.absoluteX = result.x;
					this.absoluteY = result.y;
					break;
				case "mousewheel":
				case "DOMMouseScroll":
					this.curr_wheel_delta += event.detail ? event.detail / - 3 : event.wheelDelta / 120;
					break;
				case "mouseout":
					this.absoluteX = null;
					this.absoluteY = null;
					this.relativeX = 0;
					this.relativeY = 0;
					for(i = this.history_buffer_size - 1; i > 0; i -= 1){
						this.history[i].x = 0;
						this.history[i].y = 0;
					}
					this.curr_button_states[0] = false;
					this.curr_button_states[1] = false;
					this.curr_button_states[2] = false;

					this.prev_button_states[0] = false;
					this.prev_button_states[1] = false;
					this.prev_button_states[2] = false;

					this.temp_button_states[0] = false;
					this.temp_button_states[1] = false;
					this.temp_button_states[2] = false;
					break;
				default:
					break;
			}
			Event.stop_event(event);
		},
		perform_mouse_filtering: function(x, y){
			var i,
				averageX = 0.0,
				averageY = 0.0,
				average_total = 0.0,
				current_weight = 1.0;

			for(i = this.history_buffer_size - 1; i > 0; i -= 1){
				this.history[i].x = this.history[(i - 1)].x;
				this.history[i].y = this.history[(i - 1)].y;
			}

			this.history[0].x = x;
			this.history[0].y = y;

			for(i = 0; i < this.history_buffer_size; i += 1){
				averageX += this.history[i].x * current_weight;
				averageY += this.history[i].y * current_weight;
				average_total += (1.0 * current_weight);
				current_weight *= this.weight_modifier;
			}
			this.filtered[0] = averageX / average_total;
			this.filtered[1] = averageY / average_total;
		},
		perform_mouse_smoothing: function(x, y){
			this.movementX[this.index] = x;
			this.movementY[this.index] = y;

			this.filtered[0] = (this.movementX[0] + this.movementX[1]) * 0.5;
			this.filtered[1] = (this.movementY[0] + this.movementY[1]) * 0.5;

			this.index ^= 0x01;
			this.movementX[this.index] = 0.0;
			this.movementY[this.index] = 0.0;
		},
		set_weight_modifier: function(weight_modifier){
		    this.weight_modifier = weight_modifier;
		},
		smooth_mouse: function(smooth){
			this.enable_filtering = smooth;
		},
		update: function(){
			if((new Date() - last_message)> 100){
				if(this.enable_filtering){
					this.perform_mouse_filtering(0, 0);
					this.relativeX = this.filtered[0];
					this.relativeY = this.filtered[1];

					this.perform_mouse_smoothing(this.relativeX, this.relativeY);
					this.relativeX = this.filtered[0];
					this.relativeY = this.filtered[1];
				}
			}
			this.curr_click[0] = this.temp_click[0];
			this.curr_click[1] = this.temp_click[1];
			this.curr_dbclick[0] = this.temp_dbclick[0];
			this.curr_dbclick[1] = this.temp_dbclick[1];
			this.temp_click[0] = null;
			this.temp_click[1] = null;
			this.temp_dbclick[0] = null;
			this.temp_dbclick[1] = null;

			this.prev_button_states[0] = this.curr_button_states[0];
			this.prev_button_states[1] = this.curr_button_states[1];
			this.prev_button_states[2] = this.curr_button_states[2];


			this.prev_button_states[0] = this.curr_button_states[0];
			this.prev_button_states[1] = this.curr_button_states[1];
			this.prev_button_states[2] = this.curr_button_states[2];

			this.curr_button_states[0] =(this.temp_button_states[0]) ? true : false;
			this.curr_button_states[1] =(this.temp_button_states[1]) ? true : false;
			this.curr_button_states[2] =(this.temp_button_states[2]) ? true : false;

		    this.wheel_delta = this.curr_wheel_delta - this.prev_wheel_delta;
		    this.prev_wheel_delta = this.curr_wheel_delta;
		},
		print:function(){
			var result = "";
			result += "positionX" + this.position.x + "<br />";
			result += "positionY" + this.position.y + "<br />";

			result += "relativeX" + this.relativeX + "<br />";
			result += "relativeY" + this.relativeY + "<br />";

			result += "absoluteX" + this.absoluteX + "<br />";
			result += "absoluteY" + this.absoluteY + "<br />";

			result += "wheel_delta" + this.wheel_delta + "<br />";

			result += "button_states[LEFT]" + this.curr_button_states[BUTTON_LEFT] + "<br />";
			result += "button_states[BUTTON_MIDDLE]" + this.curr_button_states[BUTTON_MIDDLE] + "<br />";
			result += "button_states[BUTTON_RIGHT]" + this.curr_button_states[BUTTON_RIGHT] + "<br />";

			result += "clickX" + this.curr_click[0] + "<br />";
			result += "clickY" + this.curr_click[1] + "<br />";

			result += "dbclickX" + this.curr_dbclick[0] + "<br />";
			result += "dbclickY" + this.curr_dbclick[1] + "<br />";
			return result;
		}
	};
}());
/*****************************************************************************
 * JavaScript �L�[�{�[�h�C�x���g�Ǘ��p
 *
 * [�L�[�{�[�h(����̃L�[�{�[�h�������ꂽ�ꍇ�̏���)keydown�ɔ������ď��������s]
 * Input.holdkey.add(element, keyname);              event�o�^
 * Input.holdkey.remove(element, keyname);           event�폜
 *
 * ���������o�^�����ꍇ�A�ȉ��̊֐��Œ�������Ԃ�����
 * Input.holdkey.is_key_on(keyname)                ����key����������Ă��邩�ǂ����H(true/false)
 *
 * [keyname�̎�ށi�������Aalt,shift,ctrl�͒P�̂ł͓o�^�ł��Ȃ�]
 * "backspace", "tab", "return", "pause","escape", "space", "pageup", "pagedown",
 * "end", "home", "left", "up","right", "down", "printscreen", "insert",
 * "delete", "f1", "f2", "f3","f4", "f5", "f6", "f7","f8", "f9", "f10", "f11",
 * "f12", "numlock", "scrolllock",
 * "0", "1", "2", "3","4", "5", "6", "7",
 * "8", "9", ";", "=","a", "b", "c", "d",
 * "e", "f", "g", "h","i", "j", "k", "l",
 * "m", "n", "o", "p","q", "r", "s", "t",
 * "u", "v", "w", "x","y", "z", "+", "-",
 *  ".", ",", ".", "/","`", "[", "\\", "]",
 ****************************************************************************/
Input.holdkey = (function(){
	//�ˑ����W���[���̓ǂݏo��
	var add = Input.event.add,
		remove = Input.event.remove,
		stop_event = Input.event.stop_event,

		Handler = {},
		map = {},//�C�x���g�Ǘ��p

		// �萔
		NOINPUT= 0,
		HOLD= 1,

		key_code_to_function_key = {
			8: "backspace",9: "tab",13: "return",19: "pause",27: "escape",
			32: "space",33: "pageup",34: "pagedown",35: "end",36: "home",
			37: "left",38: "up",39: "right",40: "down",44: "printscreen",
			45: "insert",46: "delete",112: "f1",113: "f2",114: "f3",
			115: "f4",116: "f5",117: "f6",118: "f7",119: "f8",
			120: "f9",121: "f10",122: "f11",123: "f12",144: "numlock",
			145: "scrolllock"
		},
		key_code_to_printable_char = {
			48: "0",49: "1",50: "2",51: "3",52: "4",53: "5",
			54: "6",55: "7",56: "8",57: "9",59: ";",61: "=",
			65: "a",66: "b",67: "c",68: "d",69: "e",70: "f",
			71: "g",72: "h",73: "i",74: "j",75: "k",76: "l",
			77: "m",78: "n",79: "o",80: "p",81: "q",82: "r",
			83: "s",84: "t",85: "u",86: "v",87: "w",88: "x",
			89: "y",90: "z",107: "+",109: "-",110: ".",188: ",",
			190: ".",191: "/",192: "`",219: "[",220: "\\",221: "]",
			222: "^"
		},
		key_check = function(key){
			var k;
			for(k in key_code_to_function_key){
				if(key_code_to_function_key[k] === key){
					return true;
				}
			}
			for(k in key_code_to_printable_char){
				if(key_code_to_printable_char[k] === key){
					return true;
				}
			}
			return false;
		},
		create_keyname = function(e){
			var modifiers = "",
				keyname = null,
				code = e.charCode || e.keyCode;

			keyname = key_code_to_function_key[code];

			if(!keyname){
				keyname = key_code_to_printable_char[code];
			}
			if(keyname){
				if(e.altKey){
					modifiers += "alt_";
				}
				if(e.ctrlKey){
					modifiers += "ctrl_";
				}
				if(e.shiftKey){
					modifiers += "shift_";
				}
			}else{
				return ;
			}
			return modifiers + keyname;
		},
		dispatch = function(e, key){
			var keyname = create_keyname(e),
				dkey = map[keyname];

			if(e.type !== "keydown" && e.type !== "keyup"){
				return;
			}
			if(!dkey || key !== keyname){
				return;
			}
			if(e.type === "keydown"){
				if(dkey.mode === NOINPUT){
					dkey.mode = HOLD;
					if(dkey.callback && typeof dkey.callback === 'function'){
						dkey.callback(keyname, e);
					}
				}
			}else{
				dkey.mode = NOINPUT;
			}
			stop_event(e);
			return false;
		},
		add_key_event = function(el, key){
			var keyname = key.toLowerCase();
			map[key.toLowerCase()].callback = (function(k){
				return function(e){
					return dispatch(e, k);
				};
			}(key));
			add(el, "keydown", map[keyname].callback);
			add(el, "keyup", map[keyname].callback);
		},
		remove_key_event = function(el, key){
			var keyname = key.toLowerCase();
			remove(el, "keydown", map[keyname].callback);
			remove(el, "keyup", map[keyname].callback);
		};

	Handler.add = function(el, key, callback){
		if(key_check(key)&& !map[key.toLowerCase()]){
			map[key.toLowerCase()] = {
				element: el,
				callback: callback,
				mode: NOINPUT
			};
			add_key_event(el, key);
		}
	};
	Handler.remove = function(el, key){
		if(map[key.toLowerCase()]){
			remove_key_event(el, key);
			delete map[key.toLowerCase()];
		}
	};
	Handler.is_key_on = function(key){
		var tkey = map[key.toLowerCase()];
		if(tkey && tkey.mode === HOLD){
			return true;
		}
		return false;
	};
	return Handler;
}());
/*****************************************************************************
 * JavaScript �L�[�{�[�h�C�x���g�Ǘ��p
 *
 * [�L�[�{�[�h(����̃L�[�{�[�h�������ꂽ�ꍇ�̏���)keydown�ɔ������ď��������s]
 * Input.Keybord.add(keyname);              event�o�^
 * Input.Keybord.remove(keyname);           event�폜
 * Input.Keybord.remove_event_all(keyname); event�S�폜
 *
 * [�@�\]
 * update : �L�[�{�[�h�̏�Ԃ��X�V���鏈���i���[�v�����Ŗ���ĂԕK�v������j
 *

 * [update�㗘�p�ł���@�\]
 * Input.Keybord.is_key_on(keyname)                ����key��������Ă��邩�ǂ����H(true/false)
 * Input.Keybord.is_key_triggerd(keyname)          ����key�����߂ĉ����ꂽ���ǂ����H(true/false)
 *                                           �O��UPDATE���ɉ�����ĂȂ��āA�����UPDATE��
 *                                           ���߂ĉ����ꂽ�Ƃ���true�ƂȂ�B
 *                                           (�Ȃ̂�1�񂾂������������ꍇ�Ɏg����)
 ****************************************************************************/
Input.Keybord = function(){
	var instance;

	if(!(this instanceof Input.Keybord)){
		return new Input.Keybord();
	}

	// singletron
	Input.Keybord = function(){
		return instance;
	};
	Input.Keybord.prototype = this;
	instance = new Input.Keybord();
	instance.constructor = Input.Keybord;

	this.keyStates = [
		{},
		{}
	];
	this.curr_key_states = this.keyStates[0];
	this.prev_key_states = this.keyStates[1];

	return instance;
};

Input.Keybord.prototype =(function(){
	var holdkey = Input.holdkey;
	return {
		add: function(key){
			if(this.curr_key_states[key] === undefined){
				holdkey.add(document, key);
				this.curr_key_states[key] = false;
				this.prev_key_states[key] = false;
			}
		},
		remove_event: function(key){
			if(this.curr_key_states[key] !== undefined){
				holdkey.remove(document, key);
			}
		},
		remove_event_all: function(){
			var key;
			for(key in this.curr_key_states){
				holdkey.remove(document, key);
			}
		},
		is_key_triggerd: function(key){
			return(this.curr_key_states[key] && !this.prev_key_states[key]);
		},
		is_key_on: function(key){
			return this.curr_key_states[key];
		},
		update:function(){
			var key;
			for(key in this.curr_key_states){
				this.prev_key_states[key] = this.curr_key_states[key];
				this.curr_key_states[key] = holdkey.is_key_on(key);
			}
		}
	};
}());
