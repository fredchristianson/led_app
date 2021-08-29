class Logger {
    constructor() {
        this.debugElement = document.getElementById('debug-messages');
    }
    write(message){
        console.log(message);
        const span = document.createElement('pre');
        span.innerHTML = message;
        this.debugElement.appendChild(span);
    }

    debug(message) {
      //  this.write(message);
    }

    error(message) {
        this.write(message);
    }
}

class LedSelector {
    constructor(app){
        this.count = 0;
        this.container = document.getElementById('leds');
        this.app = app;
        this.selected = [];
        
    }

    setCount(count) {
        const leds = this.container.querySelectorAll('.led');
        leds.forEach(led=>{
            if (led.dataset.index > count){
                this.container.removeChild(led);
            }
        });
        for(var i=this.count;i<count;i++) {
            const led = document.createElement('span');
            led.className = 'led';
            led.dataset.index = i;
            if (i%10 == 0) {
                led.className = 'led big';
                led.innerText = ''+i;

            }
            led.style.backgroundColor = "#000000";
            led.dataset.color = '#000000';
            this.container.appendChild(led);
        }
        this.count = count;
    }

    getCount() {
        return this.count;
    }

    setSelection(first,last){
        first = (Number.isNaN(first)) ? 9999 : first;
        last = (Number.isNaN(last)) ? -1 : last;
        this.selected = [];
        const nodes = this.container.querySelectorAll('.led');
        nodes.forEach(led=>{
            if (first == null || last == null || 1*led.dataset.index < 1*first || 1*led.dataset.index > 1*last) {
                this.app.removeClass(led,'selected');
            } else {
                this.app.addClass(led,'selected');
                this.selected.push(led);
            }
        });
    }

    rotate() {
        const nodes = this.container.querySelectorAll('.led');
        var last = nodes[nodes.length-1];
        var prev = last.dataset.color;
        nodes.forEach(cur=>{
            var tmp = cur.dataset.color;
            cur.dataset.color = prev;
            cur.style.backgroundColor = prev;
            prev = tmp;
        });
    }
    setSelectionColor(color) {
        this.selected.forEach(sel=>{
            sel.style.backgroundColor = color;
            sel.dataset.color = color;
        });
    }

    setColorAtIndex(index,color) {
        const nodes = this.container.querySelectorAll('.led');
        if (nodes[index]) {
            const c = color;
            nodes[index].style.backgroundColor = c;
            nodes[index].dataset.color = c;
        }
        this.selected.forEach(sel=>{
            sel.style.backgroundColor = color;
            sel.dataset.color = color;
        });
    }

    getRGBColors() {
        const nodes = this.container.querySelectorAll('.led');
        const rgb = [];
        nodes.forEach(node=>{
            const color = node.dataset.color || '#000000';
            const r = parseInt(color.substr(1,2),16);
            const g = parseInt(color.substr(3,2),16);
            const b = parseInt(color.substr(5,2),16);
            rgb.push({r: r, g: g, b: b});
        });
        return rgb;
    }
}


class BasicControllerApp {
    constructor() {
        this.logger = new Logger();
        this.logger.debug("BasicControllerApp running");
        this.ledSelector = new LedSelector(this);
        document.body.addEventListener('change',(changeEvent)=>{ this.handleChange(changeEvent);},true);
        document.body.addEventListener('click',(clickEvent)=>{ this.handleClick(clickEvent);},true);
        this.xhrOpen = false;
        this.initialize();
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
        const response = await this.get("config");
        if (response && response.result) {
            const config = response.data;
            this.setValue('#pin1',config.pins[0].status);
            this.setValue('#pin2',config.pins[1].status);
            this.setValue('#pin3',config.pins[2].status);
            this.setValue('#pin1-count',config.pins[0].count);
            this.setValue('#pin2-count',config.pins[1].count);
            this.setValue('#pin3-count',config.pins[2].count);
            this.setValue('#brightness',config.brightness || 30);
        }
        this.initializeTriggers();
        this.updateLedCount();
        if (response && response.result) {
            for(var idx=0;idx<response.data.leds.length;idx++){
                const led = this.toHexColor(response.data.leds[idx]);
                this.ledSelector.setColorAtIndex(idx,led);
            }
        }
    }

    isPinActive(pin) {
        const sel = document.getElementById('pin'+pin);
        return (sel && sel.value != 'off');
    }

    getPinCount(pin) {
        const sel = document.getElementById('pin'+pin+'-count');
        var count = (sel && sel.value) ? 1*sel.value : 0;
        this.logger.debug("pin count "+count);
        return count;
    }

    updateLedCount() {
        const count = [1,2,3].reduce((count,pin)=>{ 
            if (this.isPinActive(pin)) {
                count += this.getPinCount(pin);
            }
            return count;
        },0);
        this.logger.debug("total pins "+count);
        this.ledSelector.setCount(count);
    }
    getLabel(elem) {
        if (elem.id) {
            return elem.id;
        }
        var label =  elem.tagName;
        if (elem.classList && elem.classList.length>0) {
            label = label + '['+Array.from(elem.classList).join(' ')+']';
        }
        return label;
    }
    hasClass(element,name) {
        return element && element.classList && element.classList.contains(name);
    }
    initializeTriggers(){
        const triggers = document.querySelectorAll('.trigger');
        triggers.forEach((element,index) => {
            if (this.hasClass(element,'trigger-show')){
                this.triggerShow(element);
            }

        },this);
    }


    getNodes(from) {
        if (from == null) {
            return [];
        }
        if (Array.isArray(from)) {
            // assume it's a list of nodes
            return from;
        }
        if (from instanceof HTMLElement) {
            return [from];
        }
        if (typeof(from) === 'string'){
            const elements = document.querySelectorAll(from);
            return Array.from(elements);
        }
        return [];
    }

    removeClass(elements,className){
        this.getNodes(elements).forEach((element) =>{
            element.classList.remove(className);
        });
    }

    addClass(elements,className){
        this.getNodes(elements).forEach((element) =>{
            element.classList.add(className);
        });
    }

    setHandled(event) {
        event.preventDefault();
        event.stopPropagation();
    }

    setValue(selector,val) {
        const list = document.querySelectorAll(selector);
        list.forEach(elem=>{
            elem.value = (val === null || val === '') ? '' : ''+val;
        });
    }

    getValue(selector,val) {
        const list = document.querySelectorAll(selector);
        if (list && list.length>0) {
            return list[0].value;
        }
        return '';
    }

    getIntValue(selector,defaultValue=30) {
        var val = this.getValue(selector);
        if (val == null || val == '') {
            return defaultValue;
        }
        var ival = Number.parseInt(val);
        if (isNaN(ival)) {
            return defaultValue;
        }
        return ival;
    }

    setSelection(first,last) {
        this.setValue('#first-led',first);
        this.setValue('#last-led',last);
        this.ledSelector.setSelection(first,last);
    }

    handleClick(clickEvent) {
        const elem = clickEvent.target;
        this.logger.debug('click '+this.getLabel(elem));
        if (this.hasClass(elem,'tab')) {
            this.removeClass('.tabs .tab','active');
            this.removeClass('.pages .page','active');
            this.addClass(elem,'active');
            this.addClass(elem.dataset.page,'active');
            this.setHandled(clickEvent);
        }
        if (this.hasClass(elem,'select-all')){
            const count = this.ledSelector.getCount();
            this.setSelection(0,count-1);
            this.setHandled(clickEvent);
        } else if (this.hasClass(elem,'select-none')){
            this.setSelection(null,null);
            this.setHandled(clickEvent);
        }
        if (elem.id == 'save-leds') {
            this.saveLeds();
        }
        if (elem.id == 'update-leds') {
            this.sendColors();
        }

        if (elem.id == 'animate-leds') {
            this.startAnimate();
        }
        if (elem.id == 'animate-stop') {
            this.stopAnimate();
        }

        if (elem.id == 'clear-messages') {
            const nodes = document.querySelectorAll('.debug pre');
            nodes.forEach(node=>{
                node.remove();
            });
        }

    }

    startAnimate()  {
        var freq = this.getValue('#animate-speed')*1;
        if (Number.isNaN(freq) || freq < 1) {
            freq = 100;
        }
        this.stopAnimate();
        this.interval = setInterval(()=>{
            if (this.sendColors()) {
                this.ledSelector.rotate(1);
            }
        },freq);
    }

    stopAnimate() {
        if (this.interval) {
            clearInterval(this.interval);
            this.interval = null;
        }
    }



    handleChange(changeEvent) {
        const elem = changeEvent.target;
        this.logger.debug('change '+changeEvent.target.id);
        if (elem.classList && elem.classList.contains('trigger-show')) {
            this.triggerShow(elem);
        }
        if (this.hasClass(elem, 'pin-count') || this.hasClass(elem, 'pin-status')) {
            this.updateLedCount();
        }
        if (elem.name == 'led-color') {
            const cssColor = elem.value;
            this.ledSelector.setSelectionColor(cssColor);
        }
        if (elem.name == 'first' || elem.name == 'last') {
            this.ledSelector.setSelection(this.getValue('#first-led'),this.getValue('#last-led'));
        }
    }



    triggerShow(trigger) {
        if (trigger == null) {
            this.logger.error("null trigger");
            return;
        }
        if (trigger.tagName == 'SELECT')  {
            trigger = trigger.options[trigger.selectedIndex];
        }

        const hideSelector = trigger.dataset.hide;
        this.hide(hideSelector);
        const showSelector = trigger.dataset.show;
        this.show(showSelector);
    }

    show(selector) {
        if (selector == null) {
            return;
        }
        const elems = document.querySelectorAll(selector);
        elems.forEach((element) => {
            element.style.display = element.dataset.displayType || 'block';
        });
    }

    hide(selector) {
        if (selector == null) {
            return;
        }
        const elems = document.querySelectorAll(selector);
        elems.forEach((element) => {
            element.style.display = 'none';
        });
    }

    saveLeds() {
        const json = {
            "brightness": this.getIntValue('#brightness',50),
            "pins": [
                {"status": this.getValue('#pin1'),"count":this.getIntValue('#pin1-count')},
                {"status": this.getValue('#pin2'),"count":this.getIntValue('#pin2-count')},
                {"status": this.getValue('#pin3'),"count":this.getIntValue('#pin3-count')}
            ],
            "leds": this.ledSelector.getRGBColors()
        };
        this.logger.debug("JSON " + JSON.stringify(json,null,2));
        this.post('config',json);
    }

    sendColors() {
        const json = {
            "brightness": this.getIntValue('#brightness',50),
            "pins": [
                {"status": this.getValue('#pin1'),"count":this.getIntValue('#pin1-count')},
                {"status": this.getValue('#pin2'),"count":this.getIntValue('#pin2-count')},
                {"status": this.getValue('#pin3'),"count":this.getIntValue('#pin3-count')}
            ],
            "leds": this.ledSelector.getRGBColors()
        };
        this.logger.debug("JSON " + JSON.stringify(json,null,2));
        return this.post('colors',json);
    }

    get(api){
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
        
        })
    }

    post(api,json){
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
        xhr.headerData.append('Accept', '*');
        xhr.headerData.append("Access-Control-Allow", "*");
        xhr.headerData.append('Content-Type', 'text/plain');
        xhr.headerData.append('Access-Control-Allow-Origin', '*');
        xhr.headerData.append("Access-Control-Allow-Methods", "GET,HEAD,OPTIONS,POST,PUT");
        xhr.headerData.append("Access-Control-Allow-Headers", "*");
        xhr.open("POST",url,true);
        xhr.onreadystatechange =  () => {
            if (xhr.readyState == XMLHttpRequest.DONE) {
                this.xhrOpen = false;
            }
        };
        xhr.setRequestHeader("Content-Type","text/plain");

        const data = JSON.stringify(json);
        xhr.send(data);
        return true;
    }
}

export default BasicControllerApp;