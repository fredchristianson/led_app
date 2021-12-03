import Application from '../drjs/browser/application.js';
import Logger from '../drjs/logger.js';
import page from '../drjs/browser/page.js';
import httpRequest from '../drjs/browser/http-request.js';

import  util  from '../drjs/util.js';
import DOMEvent from "../drjs/browser/dom-event.js";
import DOM from "../drjs/browser/dom.js";

import { DomLogWriter } from '../drjs/browser/log-writer-dom.js';

const log = Logger.create("LedApp");



import HeaderComponent from './component/header.js';
import WhiteComponent from './component/white.js';
import SceneComponent from './component/scene.js';
import ColorComponent from './component/color.js';
import FooterComponent from './component/footer.js';
import ConfigComponent from './component/config.js';
import StripComponent from './component/strip.js';
import ScriptEditorComponent from './component/script-editor.js';
import { LOG_LEVEL } from '../drjs/log-message.js';

const PAGE_MAIN_COMPONENT = {
    "index" : WhiteComponent,
    "home" : WhiteComponent,
    "view" : WhiteComponent,
    "color" : ColorComponent,
    "scene" : SceneComponent,
    "white" : WhiteComponent,
    "scripts": ScriptEditorComponent,
    "config" : ConfigComponent
};

const LED_STRIPS = [
    {name: 'Living Room',host:"lr.400ggp.com"},
    {name: 'Dining Room',host:"dr.400ggp.com"},
    {name: 'Kitchen Cupboard',host:"kc.400ggp.com"},
    {name: 'Kitchen Floor',host:"kf.400ggp.com"},
    {name: 'Lanai',host:"lanai.400ggp.com"}
];

var NEXT_STRIP_ID=1;
class Strip {
    constructor(name,host) {
        this.name = name;
        this.host = host;
        this.id = NEXT_STRIP_ID++;
        this.config = null;
    }

    getId() { return this.id;}
    getName() { return this.name;}
    getHost() { return this.host;}

    async getConfig(){
        if (this.config) {
            return this.config;
        }
        var host = this.host;
        var url = `://${host}/api/config`;
        var configJson = await this.apiGet(url);
        try {
            this.config = JSON.parse(configJson); 
        } catch(err) {
            log.error("JSON error on config ",configJson);
            this.config = {};
        }
        return this.config;
    }

    async saveConfig(config) {
        var host = this.host;
        var url = `://${host}/api/config`;
        return await this.apiPost(url,config);
    }

    async saveScript(name,script) {
        var host = this.host;
        var url = `://${host}/api/script/${name}`;
        this.config = null;
        return await this.apiPost(url,script);
    }

    async getScript(name){
        var host = this.host;
        var url = `://${host}/api/script/${name}`;
        var script = await this.apiGet(url);
        return script;
    }

    setColor(hue,sat,light){
        var host = this.host;
        var url = `://${host}/api/std/color?hue=${hue}&saturation=${sat}&lightness=${light}`;
        log.debug("url: "+url);
        this.apiGet(url);
    }

    setWhite(brightness){
        var host = this.host;
        var url = `://${host}/api/std/white?lightness=${brightness}`;
        log.debug("url: "+url);
        this.apiGet(url);
    }

    async setOff(){
        var host = this.host;
        var url = `://${host}/api/std/off`;
        log.debug("url: "+url);
        this.apiGet(url);
    }

    async apiGet(api,params=null) {
        try {
            log.debug("GET  ",api);
            return await httpRequest.get(api,params);
        } catch(ex){
            log.error("failed to GET ",api,ex);
            return null;
        }
    };
    
    
    async apiPost(api,body) {
        try {
            log.debug("POST ",api);
            return await httpRequest.post(api,body);
        } catch(ex){
            log.error("failed to POST ",api,ex);
            return null;
        }
    };
}

export class LedApp extends Application {
    constructor() {
        super("LED App");
        new DomLogWriter('#log-container .messages',LOG_LEVEL.WARN);
        log.debug("test");
        page.setDefaultPage('white');
        this.header = new HeaderComponent('header');
        this.main = this.loadMainContent('white');
        this.strips = LED_STRIPS.map(def=>{
            return new Strip(def.name,def.host);
        });
        this.stripComponent = new StripComponent('#strips',this.strips);
        this.footer = new FooterComponent('footer');
        DOMEvent.listen('click','#main-nav a',this.onNav.bind(this));
        DOMEvent.listen('componentLoaded',this.onComponentLoaded.bind(this));
        DOMEvent.listen('click','a[href="#select-all-strips"]',this.selectAllStrips.bind(this));
        DOMEvent.listen('click','a[href="#select-no-strips"]',this.selectNoStrips.bind(this));
        DOMEvent.listen('change','.white-controls input.slider',this.setWhite.bind(this));
        DOMEvent.listen('click','.white-on ',this.setWhite.bind(this));
        DOMEvent.listen('click','.color-on ',this.setColor.bind(this));
        DOMEvent.listen('click','.scene-off ',this.setOff.bind(this));
        DOMEvent.listen('change','.color-controls input.slider',this.setColor.bind(this));
        DOMEvent.listen('click','#reload-page',this.reload.bind(this));
        DOMEvent.listen('click','#toggle-log',this.toggleLog.bind(this));
        DOMEvent.listen('click','#log-clear',this.clearLog.bind(this));
        
    }

    clearLog() {
        DOM.removeChildren("#log-container .messages");
    }
    toggleLog() {
        DOM.toggleClass('#log-container','hidden');
        DOM.toggleClass('#toggle-log','off');   
    }

    reload() {
        location.reload(true);
    };
    
    setColor() {
        var hue = DOM.getIntValue('#color-hue');
        var sat = DOM.getIntValue('#color-saturation');
        var light = DOM.getIntValue('#color-lightness');

        var sel = DOM.find('#strips input:checked');
        this.strips.filter(strip=> this.stripComponent.isSelected(strip)).forEach(strip=>{
            log.debug("set color ",strip.getName());
            strip.setColor(hue,sat,light);
        });

    }    
    
    setWhite() {
        var val = DOM.getIntValue('.white-controls .slider');
        this.strips.filter(strip=> this.stripComponent.isSelected(strip)).forEach(strip=>{
            log.debug("set white ",strip.getName());
            strip.setWhite(val);
        });

    }    
    
    setOff() {
        log.debug("turn off");
        this.strips.filter(strip=> this.stripComponent.isSelected(strip)).forEach(strip=>{
            log.debug("set off ",strip.getName());
            strip.setOff();
        });


    }

    selectAllStrips() {
        DOM.check('.strip-list input',true);
    }
    
    selectNoStrips() {
        DOM.uncheck('.strip-list input');
    }

    onComponentLoaded(component) {
        log.info("loaded component ",component.getName());
        this.setNav();
    }
    onNav(element,event) {
        var href = element.getAttribute("href");
        if (href != null && href[0] == '#') {
            var sel = href.substr(1);
            if (sel) {
                this.loadMainContent(sel);
                event.stopPropagation();
                event.preventDefault();
            }
    
        }
    }

    loadMainContent(page = null) {
        if (util.isEmpty(page)){
            page = "white";
        }
        this.currentPage = page;
        log.info("load component ",page);
        const componentHandler = PAGE_MAIN_COMPONENT[page] || WhiteComponent;
        
        this.mainComponent = new componentHandler('#main-content');
    }

    setNav() {
        DOM.removeClass("#main-nav a",'active');
        DOM.addClass("a[href='#"+this.currentPage+"']",'active');

    }


}

export default LedApp;