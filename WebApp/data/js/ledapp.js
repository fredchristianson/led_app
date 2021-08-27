import assert from './assert.js';

const LOG_LEVEL = {
    Dom: {debug:true},
    LedSelector: { debug: false}    
    

};

class Logger {
    constructor(moduleName) {
        this.moduleName = moduleName;
        this.debugElement = document.getElementById('debug-messages');
    }

    enabled(level) {
        const levels = LOG_LEVEL[this.moduleName];
        if (levels && levels[level] === false){
            return false;
        }

        return true;
    }
    write(message){
        console.log(this.moduleName+": "+message);
        const span = document.createElement('pre');
        span.innerHTML = message;
        if (this.debugElement) {
            this.debugElement.appendChild(span);
        }
    }

    debug(message) {
        if (this.enabled('debug')) {
            this.write(message);
        }
    }

    error(message) {
        if (this.enabled('error')) {
            this.write(message);
        }
    }
}

class ColorSlider {
    constructor(control,container,selector) {
        this.control = control;
        this.image = control.image;
        this.selector = selector;
        this.container = DOM.first(container);
        this.element = DOM.first(container,this.selector);
        this.moveable = this.element.parentNode;
        DOM.onTouch(this,container,selector);
        this.logger = new Logger("ColorSlider");
        this.hueRangeValue = 0;
        this.hueRangeOffset = 0;
        this.htmlHue = 0;
        this.showHue(this.hueRangeOffset);
    }

    hueRangeValueToHtmlHue(rangeValue) {
        if (this.hueMap[rangeValue]) {
            return this.hueMap[rangeValue];
        }
        const hueRangeOffset = this.image.naturalWidth*rangeValue/255;
        const colorData = DOM.getPixel(this.image,hueRangeOffset,50);
        const r = colorData[0];
        const g = colorData[1];
        const b = colorData[2];
        const htmlHue = rgb2hsv(r,g,b).hue;
        this.hueMap[rangeValue] = htmlHue;
        return htmlHue;
    }

    showHue(hueRangeOffset) {
        this.colorData = DOM.getPixel(this.image,hueRangeOffset,50);
        const r = this.colorData[0];
        const g = this.colorData[1];
        const b = this.colorData[2];
        this.rgb = {red:r,green:g, blue:b};
        this.htmlHue = rgb2hsv(r,g,b).hue;
        this.hueRangeOffset = hueRangeOffset;
        this.htmlColor = `hsl(${this.htmlHue},${this.saturation}%,${this.level}%)`;

        this.element.style.backgroundColor = this.htmlColor;
    }


    touchStart(movement,event) {
        this.logger.debug("start touch "+movement.moveTarget.id);
        this.control.sliderSelect(this,movement);
    }

    touchMove(movement,event) {
        try {
            this.logger.debug(`move touch ${movement.moveTarget.id} ${movement.moveTarget.className} ${movement.currentPos.x},${movement.lastMovement.dx}`);
            var target = movement.moveTarget;
            var pos = movement.currentPos.x;
            if (isNaN(pos)) {
                return;
            }
            while(target != null && target != this.container) {
                pos += target.offsetLeft;
                if (isNaN(pos)) {
                    return;
                }
                target = target.parentNode;
                
            }
            pos = Math.max(pos,0);
            pos = Math.min(pos,this.image.naturalWidth-1);
            this.moveable.style.left = `${pos}px`;
            this.control.sliderMove(this,pos,movement);
            this.showHue(pos);
            this.hueRangeOffset = pos;
            this.hueRangeValue = pos*255/this.image.naturalWidth;
        } catch(ex) {
            this.logger.error("move faile "+ex);
        }
    }

    touchEnd(movement,event) {
        this.logger.debug("end touch "+movement.moveTarget.id);
        this.control.sliderEndMove(this,movement);
    }

    updateColor(saturation,level) {
        this.saturation = saturation;
        this.level = level;
        this.htmlColor = `hsl(${this.htmlHue},${this.saturation}%,${this.level}%)`;
        this.element.style.backgroundColor = this.htmlColor;
    }
}

class HueSelector {
    constructor(app){

        this.hueMap = {}; // map FastLED spectrum value to HTML hsl

        this.logger = new Logger("HueSelector");
        this.section = document.querySelector('section.hsv');
        this.image = DOM.first('.hsv .hue img');
        this.selected = DOM.first(this.section,'.selected');
        this.colorElement = DOM.first(this.section,'.color');
        this.firstSlider = new ColorSlider(this,'.hsv .hue .select','.first .color');
        this.lastSlider = new ColorSlider(this,'.hsv .hue .select','.last .color');
        this.selectedSlider = null;

    }

    getHueRangeValue() { 
        return 0;//this.hueRangeValue;
    }

    getHtmlHue() {
        return 0;//this.htmlHue;
    }


    updateColor(saturation,level) {
        this.firstSlider.updateColor(saturation,level);
        this.lastSlider.updateColor(saturation,level);
    }
    toHex2(i) {
        var str = i.toString(16);
        if (str.length ==1) {
            return '0'+str;
        }
        return str;
    }

    sliderSelect(slider,movement) {
        this.selectedSlider = slider;
    }
    sliderMove(slider,movement){
    }

    sliderEndMove(slider,movement){
    }

    touchStart(movement,event) {
        this.logger.debug("touch start pos: " + event.offsetX);
        this.hueRangeValue  = this.offsetToRangeValue(event.offsetX);
       // this.logger.debug("pos: " + x);
        this.selected.style.left = ''+event.offsetX+'px';
        this.showHue(event.offsetX); 
    }

    touchMove(element,event) {
        this.hueRangeValue  = this.offsetToRangeValue(event.offsetX);
        this.logger.debug("pos: " + event.offsetX+" "  + this.hueRangeValue);
        this.selected.style.left = ''+event.offsetX+'px';
        this.showHue(event.offsetX);        
    }

    touchEnd(element,event) {
        this.logger.debug("touch end pos: " + this.offsetToRangeValue(event.offsetX)+" " + event.offsetX);
        this.app.hueChanged();
    }

    offsetToRangeValue(offset) {
        const maxWidth = this.image.naturalWidth;
        const value = Math.round(offset*255/maxWidth);
        return value;
    }
}

class LedSelector {
    constructor(app){
        this.count = 0;
        this.logger = new Logger("LedSelector");
        this.section = document.querySelector('.preview');
        this.container = document.getElementById('preview-leds');
        this.selFirst = this.section.querySelector("h2 .first");
        this.selLast = this.section.querySelector("h2 .last");
        this.selCurrent = this.section.querySelector("h2 .current");
        this.app = app;
        this.selectedStartPercent = null;
        this.selectedEndPercent = null;
        this.createLeds(100);
        DOM.html(this.selFirst,'--');
        DOM.html(this.selCurrent,'--');
        DOM.html(this.selLast,'--');
        DOM.onHover(this,'#preview-leds','.led');
        DOM.onTouch(this,'#preview-leds','.led');
        DOM.onClick('.select-all',this.selectAll.bind(this));
        DOM.onClick('.select-clear',this.selectClear.bind(this));
    }

    getStartLed() { return this.selectedStartPercent;}
    getEndLed() { return this.selectedEndPercent;}

    selectAll() {
        this.selectedStartPercent = 0;
        this.selectedEndPercent = 100;
        this.showSelectedLeds();
    }

    selectClear() {
        this.selectedStartPercent = null;
        this.selectedEndPercent = null;
        this.showSelectedLeds();
    }

    touchStart(movement,event) {
        const target = movement.moveTarget;
        if (target && target.dataset && target.dataset.index){
            const idx = 1*target.dataset.index;
            this.logger.debug("touch start "+idx);
            this.selFirst.innerHTML =""+idx;
            this.selectedStartPercent = idx;
            this.selLast.innerHTML="--";
        } else {
            this.selectedStartPercent = null;
        }
    }

    touchEnd(movement,event) {
        this.logger.debug("touch end ");
        this.showSelectedLeds();
    }

    touchMove(movement,event) {
        const target = movement.moveTarget;
        this.logger.debug("touchmove "+target.id+" " +target.className);
        if (target && target.dataset && target.dataset.index){
            const idx = 1*target.dataset.index;
            this.logger.debug("touch move "+idx);
            if (this.selectedStartPercent == null){
                this.selFirst.innerHTML =""+idx;
                this.selectedStartPercent = idx;
                this.selLast.innerHTML="--";
    
            } else {
                this.selLast.innerHTML =""+idx;
                this.selectedEndPercent = idx;
                this.selLast.innerHTML="--";
            }
            this.showSelectedLeds();
        }
    }

    showSelectedLeds() {
        const from = Math.min(this.selectedStartPercent,this.selectedEndPercent);
        const to = Math.max(this.selectedStartPercent,this.selectedEndPercent);

        this.selFirst.innerHTML =""+from;
        this.selLast.innerHTML= "" + to;

        const leds = DOM.find('#preview-leds .led');
        leds.forEach(led=>{
            if (led.dataset.index >= from && led.dataset.index <= to){
                this.logger.debug("selected "+led.dataset.index);
                DOM.addClass(led,'selected');
            } else {
                this.logger.debug("unselected "+led.dataset.index);
                DOM.removeClass(led,'selected');
            }
        });
    }

    

    updateColor(color) {
        const leds = DOM.find('#preview-leds .led.selected');
        leds.forEach(led=>{
            led.style.backgroundColor = color;
        });
    }

    setColors(colors) {
        const leds = DOM.find('#preview-leds .led');
        leds.forEach(led=>{
            var idx = led.dataset.index;
            var color = colors[idx];
            if (color != null) {
                led.style.backgroundColor = color.htmlColor;
            } else {
                led.style.backgroundColor = '#000000';
            }
        });
    }

    hoverStart(target,event) {
        const idx = 1*target.dataset.index;
        this.logger.debug("mouse over "+idx);
        this.selCurrent.innerHTML =""+idx;
    }

    hoverEnd(target,event) {
        DOM.html(this.selCurrent,'--');
    }

    setCount(count) {
        this.count = count;
    }

    getCount() {
        return this.count;
    }

    createLeds(number) {
        const leds = this.container.querySelectorAll('.led');
        leds.forEach(led=>{
            if (led.dataset.index > count){
                this.container.removeChild(led);
            }
        });
        for(var i=leds.length;i<number;i++) {
            const led = document.createElement('span');
            led.className = 'led';
            led.dataset.index = i+1;

            led.style.backgroundColor = "#000000";
            led.dataset.color = '#000000';
            this.container.appendChild(led);
        }
    }

}


class LedApp {
    constructor() {
        this.logger = new Logger("LedApp");
        this.logger.debug("LedApp running");
        this.ledSelector = new LedSelector(this);
        this.hueSelector = new HueSelector(this);
        this.xhrOpen = false;
        this.initialize();
        this.hue = 0;
        this.level = 50;
        this.saturation = 100;
        this.color = 'hsl(0,100%,100%)';
        this.color = 'ff0000';
        this.saturation = 100;
        this.levelControl = DOM.first('#level');
        this.saturationControl = DOM.first('#saturation');
        this.levelControl.value = this.level;
        this.saturationControl.value = this.saturation;
        DOM.onChange(this.levelControl,this.levelChange.bind(this));
        DOM.onChange(this.saturationControl,this.saturationChange.bind(this));
        DOM.onInput(this.levelControl,this.levelMove.bind(this));
        DOM.onInput(this.saturationControl,this.saturationMove.bind(this));
        DOM.onClick('.run-commands',this.runCommands.bind(this));
        this.zoom = 1;
        this.animateSpeedPerSecond = 0;
        this.hueSelector.updateColor(this.saturation,this.level);
    }

    levelMove(control,event) {
        this.logger.debug("level: "+ control.value);
        this.level = control.value;
        this.hueSelector.updateColor(this.saturation,this.level);
        
    }

    saturationMove(control,event) {
        this.logger.debug("saturation: "+ control.value);
        this.saturation = control.value;
        this.hueSelector.updateColor(this.saturation,this.level);
    }
    levelChange(control,event) {
        this.logger.debug("level: "+ control.value);
        this.level = control.value;
        const start = this.ledSelector.getStartLed();
        const end = this.ledSelector.getEndLed();
        this.addCommand('l',start,end,this.level,this.level,this.zoom,this.animateSpeedPerSecond);
        this.hueSelector.updateColor(this.saturation,this.level);
    }

    saturationChange(control,event) {
        this.logger.debug("saturation: "+ control.value);
        this.saturation = control.value;
        const start = this.ledSelector.getStartLed();
        const end = this.ledSelector.getEndLed();
        this.addCommand('s',start,end,this.saturation,this.saturation,this.zoom,this.animateSpeedPerSecond);
        this.updateColor();
    }

    addCommand(cmd,startLed,endLed,startValue,endValue,zoom,animateSpeed){
        if (startLed == null || endLed == null || isNaN(startLed) || isNaN(endLed)) {
            this.logger.error("invalid range "+startLed+" " +endLed);
            return;
        }
        var list = DOM.getValue('#commands');
        var cmd = `${cmd},${startLed},${endLed},${startValue},${endValue},${zoom},${animateSpeed};\n`;
        list = list + cmd;
        DOM.setValue('#commands',list);
        this.runCommands();
    }

    runCommands() {
        const leds = {};
        const sat = this.saturation;
        const level = this.level;
        var list = DOM.getValue('#commands');
        var commands = list.split(';');
        commands.forEach(cmd=>{
            const parts = cmd.trim().split(',');
            for(var p=1;p<parts.length;p++){
                var v = Number.parseFloat(parts[p]);
                if (!isNaN(v)) {
                    parts[p] = v;
                }
            }
            if (parts.length>1) {
                this.logger.debug("command: "+parts[0]);
                var led;
                for(var i=parts[1];i<=parts[2];i++) {
                    if (parts[0] == 'h'){
                        led = leds[i] || {};
                        led.hue = this.hueSelector.hueRangeValueToHtmlHue(parts[3]);
                        if (led.saturation == null) {
                            led.saturation = this.saturation;
                        }
                        if (led.level == null) {
                            led.level = this.level;
                        }
                        led.htmlColor = `hsl(${led.hue},${led.saturation}%,${led.level}%)`;
                        leds[i] = led;
                    } else if (parts[0] == 's'){
                        led = leds[i] || {};
                        if (led.level == null) {
                            led.level = level;
                        }
                        if (led.hue==null){
                            led.hue = 0;
                            led.saturation = 0;
                        }

                        led.saturation = parts[3];
                        
                        led.htmlColor = `hsl(${led.hue},${led.saturation}%,${led.level}%)`;
                        leds[i] = led;
                    } else if (parts[0] == 'l'){
                        led = leds[i] || {};
                        if (led.saturation == null) {
                            led.saturation = saturation;
                        }
                        if (led.hue==null){
                            led.hue = 0;
                            led.saturation = 0;
                        }
                        led.level = parts[3];
                        
                        led.htmlColor = `hsl(${led.hue},${led.saturation}%,${led.level}%)`;
                        leds[i] = led;
                    }

                }
            }
        });
        this.ledSelector.setColors(leds);
    }

    updateColor() {
        //this.color = `hsl(${this.htmlHue},${this.saturation}%,${this.level}%)`;
        //this.ledSelector.updateColor(this.color);
        //this.hueSelector.updateColor(this.color);
    }

    hueChanged(){
        this.htmlHue = this.hueSelector.getHtmlHue();
        this.hueRangeValue = this.hueSelector.getHueRangeValue();
        const start = this.ledSelector.getStartLed();
        const end = this.ledSelector.getEndLed();
        this.addCommand('h',start,end,this.hueRangeValue,this.hueRangeValue,this.zoom,this.animateSpeedPerSecond);
        this.updateColor();
    }

    d2h(d){
        const hex = d.toString(16);
        if (hex.length==1) {
            return '0'+hex;
        }
        return hex;
    }
    toHexColor(led) {
        const hex = `#${this.d2h(led.r)}${this.d2h(led.g)}${this.d2h(led.b)}`;
        return hex;
    }
    async initialize() {
        const responseJson = await this.get("sceen?id=123");
        if (responseJson) {
            this.config = responseJson;
            this.ledSelector.setCount(this.config.led_count);
        }
 
    }




    saveLeds() {
        const json = {
           
        };
        this.logger.debug("JSON " + JSON.stringify(json,null,2));
        this.post('config',json);
    }

    sendColors() {
        const json = {
          
        };
        this.logger.debug("JSON " + JSON.stringify(json,null,2));
        return this.post('colors',json);
    }

    get(api){
        return {
            "level": 100,
            "led_count": 273,
            "commands": [

            ]
        };
        /*
        return new Promise((resolve,reject)=>{
            const xhr = new XMLHttpRequest();
            //xhr.responseType = 'json';
            xhr.onload = (e)=>{
                const text = xhr.responseText;
                const json = JSON.parse(text);
                resolve(json);
                //resolve(xhr.response);
            };
            xhr.onerror = (e)=>{
                reject(xhr.response);
            }
            //const url = "http://192.168.10.127/api/"+api;
            const url = "/api/"+api;
            xhr.open("GET",url);
        
            xhr.send();
        
        })*/
    }

    post(api,json){
        return {

        };
        /*
        if (this.xhrOpen) {
            this.logger.error("too fast");
            if (this.resetTimeout == null) {
                this.resetTimeout = setTimeout(()=>{
                    this.resetTimeout = null;
                    this.xhrOpen = false;
                },5000);
            }
            return false;
        }
        this.xhrOpen = true;

        const xhr = new XMLHttpRequest();

        const url = "/api/"+api;
        xhr.open("POST",url,true);
        xhr.onreadystatechange =  () => {
            if (xhr.readyState == XMLHttpRequest.DONE) {
                this.xhrOpen = false;
            }
        };
        xhr.setRequestHeader("Content-Type","application/json");

        const data = JSON.stringify(json);
        xhr.send(data);
        return true;*/
    }


}

function rgb2hsv (r, g, b) {
    let rabs, gabs, babs, rr, gg, bb, h, s, v, diff, diffc, percentRoundFn;
    rabs = r / 255;
    gabs = g / 255;
    babs = b / 255;
    v = Math.max(rabs, gabs, babs),
    diff = v - Math.min(rabs, gabs, babs);
    diffc = c => (v - c) / 6 / diff + 1 / 2;
    percentRoundFn = num => Math.round(num * 100) / 100;
    if (diff == 0) {
        h = s = 0;
    } else {
        s = diff / v;
        rr = diffc(rabs);
        gg = diffc(gabs);
        bb = diffc(babs);

        if (rabs === v) {
            h = bb - gg;
        } else if (gabs === v) {
            h = (1 / 3) + rr - bb;
        } else if (babs === v) {
            h = (2 / 3) + gg - rr;
        }
        if (h < 0) {
            h += 1;
        }else if (h > 1) {
            h -= 1;
        }
    }
    return {
        hue: Math.round(h * 360),
        saturation: percentRoundFn(s * 100),
        value: percentRoundFn(v * 100)
    };
}


class Dom {

    constructor(rootSelector=null) {
        this.rootSelector = rootSelector;
        this.root = rootSelector ? document.querySelector(rootSelector) : document;
        this.listeners = [];
        this.logger = new Logger("Dom");
        Dom.NULL_ELEMENT = document.createElement('div'); // an element not on the page so anything can be done to it.  so callers don't need to test for null
    }

    removeClass(elements,className){
        this.find(elements).forEach((element) =>{
            element.classList.remove(className);
        });
    }

    addClass(elements,className){
        this.find(elements).forEach((element) =>{
            element.classList.add(className);
        });
    }


    getParentAndSelector(opts) {
        var parent=document;
        var selector='*';
        // if 1 arg is passed parent is assumed to be the document
        // and the arg is the selector
        if (opts.length == 1) {
            selector = opts[0];
        } else if (opts.length == 2) {
            parent = this.first(opts[0]);
            selector = opts[1];
        } else {
            assert.false("invalid options passed.  expect (selector) or (parent,selector)");
        }
        if (Array.isArray(parent)) {
            parent = parent.filter(elem=>{
                const validParent = elem instanceof HTMLElement || elem instanceof HTMLDocument;
                if (!validParent) {
                    // don't assert and throw an error since there are cases where Text nodes are in the array.
                    // this keeps users from needing to filter out Text nodes when looking for children.
                    this.logger.warn("parent array contains item that cannot be an HTMLElement parent");
                }
                return validParent;
            });
        } else {
            assert.type(parent,[HTMLElement,HTMLDocument],"parent must be an HTMLElement");
        }
        return {parent: parent, selector: selector};
    }

    first(...opts) {
        const sel = this.getParentAndSelector(opts);
        try {
            var element = null;
            if (sel.selector instanceof HTMLElement) {
                // a DOM element was passed as a selector, so return it
                element = sel.selector;
            } else if (Array.isArray(sel.parent)) {
                element = null;
                for(var idx=0;element == null && idx<sel.parent.length;idx++) {
                    element = sel.parent[idx].querySelector(sel.selector);
                }
                
            } else {
                element = sel.parent.querySelector(sel.selector);
            }
            return element;
        } catch(err) {
            this.logger.error("failed to find first child of selector ",sel.selector,err);
            return null;   
        }
    }

    find(...opts) {
        const sel = this.getParentAndSelector(opts);
        if (sel.selector instanceof HTMLElement) {
            // a DOM element was passed as a selector, so return it
            const element = sel.selector;
            return [element];
        } else if (Array.isArray(sel.parent)) {
            const childLists = sel.parent.map(parent=>{
                return Array.from(parent.querySelectorAll(sel.selector));
            });
            return [].concat(...childLists);
        } else {
            const elements = sel.parent.querySelectorAll(sel.selector);
            return Array.from(elements);
        }
        
    }

    hasClass(element,name) {
        return element && element.classList && element.classList.contains(name);
    }

    html(element,html=null) {
        if (html) {
            this.find(element).forEach(e=>{e.innerHTML=html;})
        } else {
            return this.first(element).innerHTML;
        }
    }

    onHover(handler,parent,selector=null) {
        if (selector == null) {
            selector = parent;
        }
        var instance = null;
        var startHandler=handler;
        var endHandler=handler;
        if (typeof(handler) != 'function') {
            instance = handler;
            startHandler = instance['hoverStart'].bind(instance);
            endHandler = instance['hoverEnd'].bind(instance);
        }
        this.listen('mouseover',parent,(event)=>{
            const target = event.target;
            if (this.match(target,selector)){
                startHandler(target,event);
            }
        });
        this.listen('mouseout',parent,(event)=>{
            const target = event.target;
            if (this.match(target,selector)){
                endHandler(target,event);
            }
        });

    }

    onTouch(handler,parent,selector=null) {
        parent = this.first(parent);
        var instance = null;
        var startHandler=handler;
        var endHandler=handler;
        var moveHandler = handler;
        if (typeof(handler) != 'function') {
            instance = handler;
            startHandler = instance['touchStart']?.bind(instance);
            endHandler = instance['touchEnd']?.bind(instance);
            moveHandler = instance['touchMove']?.bind(instance);
        }

        this.listen('mousedown',parent,(event)=>{
            var self = this;
            const movement = {
                startPos: {x:event.offsetX,y:event.offsetY},
                currentPos: {x:event.offsetX,y:event.offsetY},
                lastPos: {x:event.offsetX,y:event.offsetY},
                totalMovement: {dx:0,dy:0},
                lastMovement: {dx:0,dy:0},
                container: parent,
                moveTarget: event.target
            };
            function move(event) {
                const target =  event.target;
                movement.moveTarget = target;
                movement.lastPos = movement.currentPos;
                movement.currentPos = {x:event.offsetX,y:event.offsetY};
                movement.lastMovement = {dx:movement.currentPos.x-movement.lastPos.x,dy:movement.currentPos.y-movement.lastPos.y}
                movement.totalMovement = {dx:movement.currentPos.x-movement.startPos.x,dy:movement.startPos.y-movement.lastPos.y}
                self.logger.debug("mousemove "+target.id);
                moveHandler(movement,event);
                
                return true;
            }

            function up(event){
                const target = event.target;
                movement.moveTarget = target;
                self.logger.debug("mouseup "+target.id);
                document.removeEventListener('mouseup',up);
                document.removeEventListener('mousemove',move);
                endHandler(movement,event);
                return true;
            }

            const target = event.target;
            if (target == parent || this.match(target,selector)) {
                this.logger.debug("mousedown "+target.id);
                if (startHandler) {
                    startHandler(movement,event);
                }
                document.addEventListener('mousemove',move);
                document.addEventListener('mouseup',up);
                return true;
            } else {
                this.logger.debug("mousedown not matched "+target.id);
                return false;
            }
        });

        //this.touchHandlers(parent,selector,startHandler,moveHandler,endHandler);
    }

    touchHandlers(parentOrSelector,selector,startHandler,moveHandler,endHandler) {
        var touching = false;
        const parent = this.first(parentOrSelector);
        this.logger.debug("touch handlers "+selector+"  "+touching);
        if (startHandler) {
            this.listen('mousedown',parent,(event)=>{
                touching = true;
                this.logger.debug("set touching "+selector);
                const target = event.target;
                if (this.match(target,selector)){
                    startHandler(target,event);
                    return true;
                }
                return false;
            });
        }
        if (endHandler) {
            this.listen('mouseup',document.body,(event)=>{
                const target = event.target;
                this.logger.debug("mouseup "+(touching ? 'is' : 'is not')+" touching ");
                if (touching){
                    touching = false;
                    endHandler(target,event);
                    return true;
                }
                this.logger.debug("set not touching ");
                return false;
            });
        }
        if (moveHandler) {
            this.listen('mousemove',parent,(event)=>{
                this.logger.debug("mousemove "+(touching ? 'is' : 'is not')+" touching ");
                const target = event.target;
                if (touching && this.match(target,selector)){
                    moveHandler(target,event);
                    return true;
                }
                return false;
            });
        }
    }

    match(elem,selector) {
        if (elem == selector || selector == null) {
            return true;
        }
        return elem && elem.matches(selector);
    }

    onChange(selector,handler) {
        this.listen('change',selector,(event)=>{
            const target = event.target;
            if (this.match(target,selector)) {
                var value = target.value;
                var type = target.type;
                if (target.type == 'number' || target.type == 'range') {
                    value = 1*value;
                }
                handler(target,event,value);
            }
        });
    }
    onInput(selector,handler) {
        this.listen('input',selector,(event)=>{
            const target = event.target;
            if (this.match(target,selector)) {
                var value = target.value;
                var type = target.type;
                if (target.type == 'number' || target.type == 'range') {
                    value = 1*value;
                }
                handler(target,event,value);
            }
        });
    }
    
    onClick(selector,handler) {
        this.listen('click',selector,(event)=>{
            const target = event.target;
            if (this.match(target,selector)) {
                handler(target,event);
            }
        });
    }

    listen(eventType,sel,handler) {
        const elements = this.find(sel);
        
        elements.forEach(elem=>{
            var listener = {
                type: eventType,
                element: elem,
                handler: (event)=>{
                    var handled = handler(event); 
                    if (typeof handled == 'undefined' || handled) {
                        event.stopPropagation();
                        event.preventDefault();
                    }
                }
            };
            elem.addEventListener(eventType,listener.handler);
            this.listeners.push(listener);
        });
    }

    getPixel(imgElement,x,y) {
        const img = this.first(imgElement);
        if (this.canvas == null) {
            this.canvas = document.createElement('canvas');
        }

        this.canvas.width = img.naturalWidth;
        this.canvas.height = img.naturalHeight;
        //canvas.getContext('2d').drawImage(img, x, y, 1, 1, 0, 0, 1, 1);
        this.canvas.getContext('2d').drawImage(img, 0,0,img.naturalWidth,img.naturalHeight);
        let pixelData = this.canvas.getContext('2d').getImageData(x,y, 1, 1).data;
    
        return pixelData; 
    }

    getValue(selector) {
        const elem = this.first(selector);
        return elem.value;
    }

    setValue(selector,value) {
        const elem = this.first(selector);
        elem.value = value;
    }
}

const DOM = new Dom();


export default LedApp;