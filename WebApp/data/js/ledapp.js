//import {assert} from './assert.js';

const API_BASE_DINING_ROOM = 'http://192.168.10.133/api/';
const API_BASE_LIVING_ROOM = 'http://192.168.10.120/api/';
const API_BASE_SAME_HOST = '/api/';
const API_BASE = API_BASE_SAME_HOST;

class Api {
    constructor() {
        this.baseUrl = API_BASE;
    }

    async get(path) {
        return new Promise((resolve,reject)=>{
            const xhr = new XMLHttpRequest();
            //xhr.responseType = 'json';
            xhr.onload = (e)=>{
                const text = xhr.responseText;
                resolve(text);
                //resolve(xhr.response);
            };
            xhr.onerror = (e)=>{
                reject(xhr.response);
            };
            //const url = "http://192.168.10.127/api/"+api;
            const url = this.baseUrl + path;
            xhr.open("GET",url);
        
            xhr.send();
        
        })
    }

    async getJSON(path,data) {
        const response = await this.get(path,data);
        const json = JSON.parse(response);
        return json;
    }

    async post(path,data) {
        return new Promise((resolve,reject)=>{
            const xhr = new XMLHttpRequest();

            const url = this.baseUrl+path;
            xhr.open("POST",url,true);
            xhr.onreadystatechange =  () => {
                if (xhr.readyState == XMLHttpRequest.DONE) {
                    resolve(xhr.resonseText);
                }
            };
            xhr.onerror = (e)=>{
                reject(xhr.response);
            };
            xhr.setRequestHeader("Content-Type","application/json");

            xhr.send(data);
            return true;
        });
    }
}


const LOG_LEVEL = {
    Dom: {debug:false},
    LedSelector: { debug: false}, 
    ColorSlider: {debug: true}

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
    write(message,levelName='unknown'){
        console.log(this.moduleName+"("+levelName+"): "+message);
        const span = document.createElement('pre');
        span.innerHTML = message;
        if (this.debugElement) {
            this.debugElement.appendChild(span);
        }
    }

    debug(message) {
        if (this.enabled('debug')) {
            this.write(message,'debug');
        }
    }

    warn(message) {
        if (this.enabled('warn')) {
            this.write(message,'warn');
        }
    }


    error(message, exception=null) {
        if (this.enabled('error')) {
            this.write(message,'error');
            if (exception) {
                this.write(exception);
                if (exception.stack){
                    this.write(exception.stack);
                }
            }
        }
    }
}

class Observable {
    constructor() {
        this.observers = [];
    }

    observe(observer) {
        this.observers.push(observer);
    }

    changed() {
        this.observers.forEach(observer=>{
            observer(this);
        });
    }
}

class Gradient {
    constructor(startValue, endValue) {
        this.startValue = startValue;
        this.endValue = endValue;
    }

    setValue(value) { throw new Exception("setValue() not implemented");}

    apply(leds,startPercent,endPercent){
        if (startPercent == null || startPercent < 0 || endPercent == null || endPercent < 0) {
            return;
        }
        const ledCount = leds.length;
        const startIndex = Math.floor(startPercent*ledCount/100);
        const endIndex = Math.floor(endPercent*ledCount/100);
        const valueCount = (this.endValue-this.startValue)+1.0;
        for(var i=startIndex;i<leds.length && i<=endIndex;i++) {
            const pct = (i-startIndex)/ledCount;
            const increment = pct*valueCount;
            this.setValue(leds[i],this.startValue+increment);
        }
    }
}

class HueGradient extends Gradient {
    constructor(startValue,endValue) {
        super(startValue,endValue);
    }
    setValue(led,value) {
        led.hue = colorMgr.hueRangeValueToHtmlHue(value);
    }
}

class SaturationGradient extends Gradient {
    constructor(startValue,endValue) {
        super(startValue,endValue);
    }
    setValue(led,value) {
        led.saturation = value;
    }
}

class LevelGradient extends Gradient {
    constructor(startValue,endValue) {
        super(startValue,endValue);
    }
    setValue(led,value) {
        led.level = value;
    }
}



class HslColor extends Observable {
    constructor(colorMgr){
        super();
        this.colorMgr = colorMgr;
        this.hueRangeValue = 0;
        this.htmlHue = 0;
        this.saturation = 100;
        this.level = 50;
        this.ledPercent = null;
    }

    setLedPercent(percent) {
        this.ledPercent = percent;
    }

    getLedPercent() {
        return this.ledPercent;
    }

    setHueRangeValue(value) {
        this.hueRangeValue = value;
        this.htmlHue = this.colorMgr.hueRangeValueToHtmlHue(value);
        this.changed();
        this.colorMgr.changed();
    }

    setSaturation(value) {
        this.saturation = value;
        this.changed();
        this.colorMgr.changed();
    }
    setLevel(value) {
        this.level = value;
        this.changed();
        this.colorMgr.changed();
    }

    getHueRangeValue() {
        return this.hueRangeValue;
    }
    getHtmlHue() {
        return this.htmlHue;
    }

    getSaturation() { 
        return this.saturation;
    }

    getLevel() {
        return this.level;
    }

    getHsl() {
        const saturation = this.colorMgr.saturationEnabled ? this.saturation : 100;
        const level = this.colorMgr.levelEnabled ? this.level : 50;
        this.htmlColor = `hsl(${this.htmlHue},${saturation}%,${level}%)`;
        return this.htmlColor;
    }
}

class ColorManager extends Observable {
    constructor() {
        super();
        this.first = new HslColor(this);
        this.last = new HslColor(this);
        this.image = DOM.first('#image-spectrum');
        this.hueMap = {};
        this.hueEnabled = true;
        this.saturationEnabled = true;
        this.levelEnabled = true;
        this.gradientEnabled = false;
    }

    enableGradient(isEnabled=true) { this.gradientEnabled = isEnabled; this.changed();}

    disableHue() { this.hueEnabled = false; this.changed();}
    enableHue(isEnabled=true) { this.hueEnabled = isEnabled; this.changed();}
    disableSaturation() { this.saturationEnabled = false; this.changed();}
    enableSaturation(isEnabled=true) { this.saturationEnabled = isEnabled; this.changed();}
    disableLevel() { this.levelEnabled = false; this.changed();}
    enableLevel(isEnabled=true) { this.levelEnabled = isEnabled; this.changed();}
    setSelectedRange(firstPercent,lastPercent) {
        this.first.setLedPercent(firstPercent);
        this.last.setLedPercent(lastPercent);
        this.changed();
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

    getStartHslColor(){
        return this.first;
    }
    getEndHslColor(){
        return this.gradientEnabled ? this.last : this.first;
    }


    updateColors(leds) {
        const gradient = this.gradientEnabled;
        const start = this.first;
        const end = gradient ? this.last : this.first;
        const hueGradient = new HueGradient(start.getHueRangeValue(),end.getHueRangeValue());
        const levelGradient = new LevelGradient(start.getLevel(),end.getLevel());
        const saturationGradient = new SaturationGradient(start.getSaturation(),end.getSaturation());
        if (this.hueEnabled) {
            // use this.last led percent instead of end led percent because last always has the end of selection
            hueGradient.apply(leds,start.getLedPercent(),this.last.getLedPercent());
        }
        if (this.levelEnabled) {
            levelGradient.apply(leds,start.getLedPercent(),this.last.getLedPercent());
        }
        if (this.saturationEnabled) {
            saturationGradient.apply(leds,start.getLedPercent(),this.last.getLedPercent());
        }
        leds.forEach(led=>{
            if (led.hue != null) {
                led.htmlColor = `hsl(${led.hue},${led.saturation === null ? 100 : led.saturation}%,${led.level===null ? 50 : led.level}%)`;
            } else {
                led.htmlColor = "hsl(0,0%,0%)";
            }
        });
    }
}

class ColorSlider {
    constructor(control,container,selector,hslColor) {
        this.control = control;
        this.image = control.image;
        this.selector = selector;
        this.hslColor = hslColor;
        hslColor.observe(this.onColorChanged.bind(this));
        colorMgr.observe(this.onColorChanged.bind(this));
        this.container = DOM.first(container);
        this.element = DOM.first(container,this.selector);
        this.moveable = this.element.parentNode;
        DOM.onTouch(this,container,selector);
        this.logger = new Logger("ColorSlider");
        this.hueRangeValue = 0;
        this.hueRangeOffset = 0;
        this.htmlHue = 0;
        this.hslColor.setHueRangeValue(this.hueRangeValue);
    }

    touchStart(movement,event) {
        this.logger.debug("start touch "+movement.moveTarget.id);
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
            this.hueRangeOffset = pos;
            this.hueRangeValue = Math.round(pos*255/this.image.naturalWidth);
            this.hslColor.setHueRangeValue(this.hueRangeValue);
            this.logger.debug(`hue: ${pos}, ${this.image.naturalWidth}, ${this.hueRangeValue}`);
        } catch(ex) {
            this.logger.error("move failed ",ex);
        }
    }

    touchEnd(movement,event) {
        this.logger.debug("end touch "+movement.moveTarget.id);
    }

    onColorChanged(observable) {
        this.element.style.backgroundColor = this.hslColor.getHsl();
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
        this.firstSlider = new ColorSlider(this,'.hsv .hue .select','.first .color',colorMgr.first);
        this.lastSlider = new ColorSlider(this,'.hsv .hue .select','.last .color',colorMgr.last);
        this.selectedSlider = null;
    }

    getHueRangeValue() { 
        return 0;//this.hueRangeValue;
    }

    getHtmlHue() {
        return 0;//this.htmlHue;
    }

    toHex2(i) {
        var str = i.toString(16);
        if (str.length ==1) {
            return '0'+str;
        }
        return str;
    }

}

class LedSelector {
    constructor(app){
        this.count = 101;
        this.logger = new Logger("LedSelector");
        this.section = document.querySelector('.preview');
        this.container = document.getElementById('preview-leds');
        this.selFirst = this.section.querySelector("h2 .first");
        this.selLast = this.section.querySelector("h2 .last");
        this.selCurrent = this.section.querySelector("h2 .current");
        this.app = app;
        this.selectedStartIndex = null;
        this.selectedEndIndex = null;
        this.createLeds(this.count);
        DOM.html(this.selFirst,'--');
        DOM.html(this.selCurrent,'--');
        DOM.html(this.selLast,'--');
        DOM.onTouch(this,'#preview-leds','.led');
        DOM.onClick('.select-all',this.selectAll.bind(this));
        DOM.onClick('.select-clear',this.selectClear.bind(this));

    }

    getStartLed() { return this.selectedStartIndex;}
    getEndLed() { return this.selectedEndIndex;}
    getStartLedPercent() { return this.ledElements[this.selectedStartIndex].dataset.percent;}
    getEndLedPercent() { return this.ledElements[this.selectedEndIndex].dataset.percent;}

    selectAll() {
        this.selectedStartIndex = 0;
        this.selectedEndIndex = this.ledElements.length-1;
        this.showSelectedLeds();
    }

    selectClear() {
        this.selectedStartIndex = null;
        this.selectedEndIndex = null;
        this.showSelectedLeds();
    }

    touchStart(movement,event) {
        const target = movement.moveTarget;
        if (target && target.dataset && target.dataset.index){
            const idx = 1*target.dataset.index;
            this.logger.debug("touch start "+idx);
            this.selFirst.innerHTML =""+idx;
            this.selectedStartIndex = idx;
            this.selLast.innerHTML="--";
        } else {
            this.selectedStartIndex = null;
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
            if (this.selectedStartIndex == null){
                this.selFirst.innerHTML =""+idx;
                this.selectedStartIndex = idx;
                this.selLast.innerHTML="--";
    
            } else {
                this.selLast.innerHTML =""+idx;
                this.selectedEndIndex = idx;
                this.selLast.innerHTML="--";
            }
            this.showSelectedLeds();
        }
    }

    showSelectedLeds() {
        if (this.selectedStartIndex == null || this.selectedEndIndex==null){
            colorMgr.setSelectedRange(null,null);
            DOM.removeClass('#preview-leds .led','selected');
            return;
        }
        const from = Math.min(this.selectedStartIndex,this.selectedEndIndex);
        const to = Math.max(this.selectedStartIndex,this.selectedEndIndex);

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
        colorMgr.setSelectedRange(leds[from].dataset.percent,leds[to].dataset.percent);
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



    setCount(count) {
        //this.count = count;
        this.logger.warn("cannot set led count any more.  always use 101 for 0-100%");
    }

    getCount() {
        return this.count;
    }

    createLeds(number) {
        var leds = this.container.querySelectorAll('.led');
        leds.forEach(led=>{
            if (led.dataset.index > this.count){
                this.container.removeChild(led);
            }
        });
        for(var i=leds.length;i<number;i++) {
            const led = document.createElement('span');
            led.className = 'led';
            led.dataset.index = i;

            led.style.backgroundColor = "#000000";
            led.dataset.color = '#000000';
            this.container.appendChild(led);
        }
        leds = DOM.find('#preview-leds .led');
        const total = leds.length;
        for(i=0;i<total;i++) {
            leds[i].dataset.percent = Math.round(i/(total-1)*100);
        }
        this.ledElements = leds;
    }


}

class InputRangeSelector {
    constructor(selector){
        this.elem = DOM.first(selector);
        DOM.onInput(this.elem,this.onChange.bind(this));
        DOM.onChange(this.elem,this.onSelected.bind(this));
    }

    changed(value) {
        alert("changed(value) not implemented");
    }
    onChange(element,event) {
        this.changed(DOM.getValue(element));
    }

    onSelected(element,event) {
        this.changed(DOM.getValue(element));
    }
}

class SaturationSelector extends InputRangeSelector {
    constructor(selector,hslColor){
        super(selector);
        this.hslColor = hslColor;
    }

    changed(value){
        this.hslColor.setSaturation(value);
    }
}


class LevelSelector extends InputRangeSelector {
    constructor(selector,hslColor){
        super(selector);
        this.hslColor = hslColor;
    }

    changed(value){
        this.hslColor.setLevel(value);
    }
}


export class LedApp {
    constructor() {
        this.logger = new Logger("LedApp");
        this.logger.debug("LedApp running");
        this.ledSelector = new LedSelector(this);
        this.hueSelector = new HueSelector(this);
        this.xhrOpen = false;

        DOM.onClick('.run-commands',this.runCommands.bind(this));
        this.zoom = 1;
        this.animateSpeedPerSecond = 0;
        DOM.onClick('#gradient',this.toggleGradient.bind(this));
        DOM.onClick('#hue-off',this.toggleHue.bind(this));
        DOM.onClick('#level-off',this.toggleLevel.bind(this));
        DOM.onClick('#saturation-off',this.toggleSaturation.bind(this));
        this.saturationFirst = new SaturationSelector('#saturation-first',colorMgr.first);
        this.saturationLast = new SaturationSelector('#saturation-last',colorMgr.last);
        this.levelFirst = new LevelSelector('#level-first',colorMgr.first);
        this.levelLast = new LevelSelector('#level-last',colorMgr.last);
        colorMgr.observe(this.onColorChanged.bind(this));

        DOM.onClick('.create-command.hue',this.createHueCommand.bind(this));
        DOM.onClick('.create-command.level',this.createLevelCommand.bind(this));
        DOM.onClick('.create-command.saturation',this.createSaturationCommand.bind(this));
        DOM.onClick('.create-command.add-hsl',this.createHSLCommands.bind(this));
        this.initialize();

    }

    onColorChanged() {
        this.runCommands();
    }
    
    isGradient() {
        const elem = DOM.first("#gradient");
        return elem.checked;
    }
    toggleHue(elem,event) {
        if (elem.checked) {
            DOM.addClass(document.body,'no-hue');
            colorMgr.disableHue();
        } else {
            colorMgr.enableHue();
            DOM.removeClass(document.body,'no-hue');
        }
        colorMgr.changed();
        return false;
    }  
    
    toggleSaturation(elem,event) {
        DOM.toggleClass(document.body,'no-saturation');
        colorMgr.enableSaturation(!elem.checked);
        return false;
    }  

    toggleLevel(elem,event) {
        DOM.toggleClass(document.body,'no-level',elem.checked);
        colorMgr.enableLevel(!elem.checked);
        return false;
    }  

    toggleGradient(elem,event) {
        DOM.toggleClass(document.body,'gradient',elem.checked);
        colorMgr.enableGradient(elem.checked);
        return false;
    }
  

    createHueCommand() {
        const start = colorMgr.getStartHslColor();
        const end = colorMgr.getEndHslColor();
        this.addCommand('h',this.ledSelector.getStartLedPercent(),this.ledSelector.getEndLedPercent(),
                            start.getHueRangeValue(),end.getHueRangeValue(),
                            this.zoom,this.animateSpeedPerSecond);
    }

    createLevelCommand() {
        const start = colorMgr.getStartHslColor();
        const end = colorMgr.getEndHslColor();
        this.addCommand('l',this.ledSelector.getStartLedPercent(),this.ledSelector.getEndLedPercent(),
                            start.getLevel(),end.getLevel(),
                            this.zoom,this.animateSpeedPerSecond);
    }
    createSaturationCommand() {
        const start = colorMgr.getStartHslColor();
        const end = colorMgr.getEndHslColor();
        this.addCommand('s',this.ledSelector.getStartLedPercent(),this.ledSelector.getEndLedPercent(),
                            start.getSaturation(),end.getSaturation(),
                            this.zoom,this.animateSpeedPerSecond);
    }

    createHSLCommands() {
        if (colorMgr.hueEnabled){
            this.createHueCommand();
        }
        if (colorMgr.levelEnabled){
            this.createLevelCommand();
        }    
        if (colorMgr.saturationEnabled){
            this.createSaturationCommand();
        }
    }

    addCommand(cmd,startPercent,endPercent,startValue,endValue,zoom,animateSpeed){
        if (startPercent == null || endPercent == null || isNaN(startPercent) || isNaN(endPercent)) {
            this.logger.error("invalid range "+startPercent+" " +endPercent);
            return;
        }
        var list = DOM.getValue('#scene');
        var cmd = `${cmd},${startPercent},${endPercent},${startValue},${endValue},${zoom},${animateSpeed};\n`;
        list = list + cmd;
        DOM.setValue('#scene',list);
        this.runCommands();
    }

    runCommands() {
        const leds = [];
        for(var c=0;c<this.ledSelector.getCount();c++) {
            leds[c] = {hue:null, level:null,saturation:null,htmlColor:null};
        }
        const sat = this.saturation;
        const level = this.level;
        var list = DOM.getValue('#scene');
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
                const cmd = parts[0];
                const startPercent = parts[1];
                const endPercent = parts[2];
                var gradient = null;
                if (cmd == 'h') {
                    gradient = new HueGradient(parts[3],parts[4]);
                } else if (cmd == 'l') {
                    gradient = new LevelGradient(parts[3],parts[4]);
                } else if (cmd == 's') {
                    gradient = new SaturationGradient(parts[3],parts[4]);
                }
                if (gradient) {
                    gradient.apply(leds,startPercent,endPercent);
                } else {
                    this.logger.debug("unknownn command: "+cmd);
                }
                
            }
        });
        colorMgr.updateColors(leds);
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
        DOM.onClick('#save-scene',this.saveCommands.bind(this));
        DOM.onClick('#load-scene',this.loadCommands.bind(this));
        this.api = new Api();
        await this.readConfig();
    }

    async readConfig(){
         const response = await this.api.getJSON("scene/config");
        if (response) {
            this.config = response;
            this.ledSelector.setCount(this.config.led_count);
            DOM.setHtml('#room-name',this.config.hostname + " - " + this.config.ip_addr);
            DOM.setSelectOptions('#saved-scenes',this.config.scenes,null); //,DOM.getValue('#scene-name'));
            this.ledSelector.selectClear();
            this.runCommands();
        } else {
            alert("Failed to get configuration");
        }
 
    }


    async loadCommands() {
        const name = DOM.getValue('#saved-scenes');
        try {
            const scene = await this.api.get(`scene/${name}`);
            DOM.setValue('#scene',scene);
            DOM.setValue('#scene-name',name);
            this.ledSelector.selectClear();
            this.runCommands();
        } catch(ex) {
            this.logger.error("load failed ",ex);
        }
    }

    async saveCommands() {
        const name = DOM.getValue('#scene-name');
        const commands = DOM.getValue('#scene');
        DOM.setValue('#saved-scenes',name);
        try {
            await this.api.post(`scene/${name}`,commands);
        } catch(ex) {
            this.logger.error("POST failed",ex);
            alert("save failed");
        }
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

    toggleClass(elements,className,isSet=null){
        var elements = this.find(elements);

        elements.forEach((element) =>{
            var add = isSet;
            if (isSet === null) {
                add = !this.hasClass(element,className);
            }
            if (add){
                element.classList.add(className);
            } else {
                element.classList.remove(className);
            }
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
            alert("invalid options passed.  expect (selector) or (parent,selector)");
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
            //assert.type(parent,[HTMLElement,HTMLDocument],"parent must be an HTMLElement");
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
            startHandler = instance['touchStart'] ? instance['touchStart'].bind(instance) : null;
            endHandler = instance['touchEnd']?instance['touchEnd'].bind(instance) : null;
            moveHandler = instance['touchMove']?instance['touchMove'].bind(instance) : null;
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
               return handler(target,event,value);
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
                return handler(target,event,value);
            }
        });
    }
    
    onClick(selector,handler) {
        this.listen('click',selector,(event)=>{
            const target = event.target;
            if (this.match(target,selector)) {
                return handler(target,event);
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
        if (elem.type == 'range' || elem.type == 'number') {
            return Number.parseFloat(elem.value);
        } else if (elem.tagName=='SELECT') {
            const options = this.find(elem,'option');
            for(var i=0;i<options.length;i++) {
                if (options[i].selected) {
                    return options[i].value;
                }
            }
        }
        return elem.value;
    }

    setValue(selector,value) {
        const elems = this.find(selector);
        elems.forEach(elem=>{
            if (elem.tagName == 'SELECT') {
                const opts = this.find(elem,'option');
                var found = null;
                opts.forEach(opt=>{
                    if (opt.value == value) {
                        found = opt;
                        opt.selected = true;
                    } else {
                        opt.selected = false;
                    }
                });
                if (found == null) {
                    this.addOption(elem,value,true);
                }
            } else {
                elem.value = value;
            }
        });
    }

    setHtml(selector,innerHtml){
        const elements = this.find(selector);
        elements.forEach(elem=>{
            elem.innerHTML = innerHtml;
        });
    }

    addOption(select,opt,selected=false) {
        this.find(select).forEach(sel=>{
            var option = document.createElement('option');
            option.value = opt;
            option.innerHTML = opt;
            if (selected){
                option.setAttribute('selected','selected');
            }
            select.appendChild(option);
        })
    }
    setSelectOptions(selector,options,selectedOption,defaultOption='--select--') {
        const elements = DOM.find(selector);
        elements.forEach(select=>{
            select.innerHTML='';
            if (defaultOption) {
                this.addOption(select,defaultOption);
            }
            var selected = null;
            options.forEach(opt=>{
                this.addOption(select,opt,opt == selectedOption);
            });
        });

    }
};

const DOM = new Dom();

const colorMgr = new ColorManager();

export default LedApp;