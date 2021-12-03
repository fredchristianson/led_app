import {ComponentBase} from '../../drjs/browser/component.js';
import assert from '../../drjs/assert.js';
import util from '../../drjs/util.js';
import DOM from '../../drjs/browser/dom.js';
import  DOMEvent from '../../drjs/browser/dom-event.js';
import Logger from '../../drjs/logger.js';

const log = Logger.create("ScriptEditor");

import {HtmlTemplate, HtmlValue,TextValue,AttributeValue,DataValue } from '../../drjs/browser/html-template.js';

export class ScriptEditorComponent extends ComponentBase{
    constructor(selector, strips, htmlName='script') {
        super(selector,htmlName);
        this.strip = null;
        DOMEvent.listen('singleStripSelection',this.onStripSelected.bind(this));
        DOMEvent.listen('click','.save-script',this.onSave.bind(this));
        DOMEvent.listen('click','.load-script',this.onLoad.bind(this));
        DOMEvent.listen('change','.select-script',this.onSelect.bind(this));
    }

    async onStripSelected(strip){
        this.strip = strip;
        if (strip == null) {
            DOM.hide(this.elements);
            return;
        }
        log.debug("strip selected ",strip.getName());
        DOM.show(this.elements);
        var config = await this.strip.getConfig();

        DOM.setValue(this.text,'');
        DOM.setOptions(this.select,config.scripts,"--select--");
    }

    async onSelect(element,event) {
        var name = DOM.getValue(element);
        var text = "";
        if (name != null) {
            DOM.setValue(this.name,name);
            text = await this.strip.getScript(name);
        }
        DOM.setValue(this.text,text);
        
        
        return DOMEvent.HANDLED;
    }

    onSave(element,event) {
        var name = DOM.getValue(this.name);
        log.debug("save ",this.nameElement);
        var val = DOM.getValue("#script-text");
        this.strip.saveScript(name,val);
        return DOMEvent.HANDLED;
    }

    onLoad(element,event) {
        var name = DOM.getValue(this.nameElement);
        return DOMEvent.HANDLED;
    }

    async onAttached(elements,parent){
        this.elements = elements;
        this.text = DOM.first(parent,'#script-text');
        this.name = DOM.first(parent,'.script [name="name"]');
        this.select = DOM.first(parent,'.select-script');
        
        this.loaded = true;
        this.onStripSelected(this.strip);

    }

}

export default ScriptEditorComponent;