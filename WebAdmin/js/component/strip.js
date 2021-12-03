import {ComponentBase} from '../../drjs/browser/component.js';
import assert from '../../drjs/assert.js';
import util from '../../drjs/util.js';
import DOM from '../../drjs/browser/dom.js';
import  DOMEvent from '../../drjs/browser/dom-event.js';
import Pager from './pager.js';

import {HtmlTemplate, HtmlValue,TextValue,AttributeValue,DataValue } from '../../drjs/browser/html-template.js';

export class StripComponent extends ComponentBase{
    constructor(selector, strips, htmlName='strips') {
        super(selector,htmlName);
        this.strips = strips;
        this.stripSelectionType = null;
        this.selectedValue = null;
        this.select = null;
        DOMEvent.listen('componentLoaded',this.onComponentLoaded.bind(this));
        DOMEvent.listen('input','#select-strip',this.changeSelection.bind(this));
    }

    changeSelection(element,event){
        var val = DOM.getValue(element);
        this.selectedValue = val;
        DOMEvent.trigger('singleStripSelection',val,this);
    }
    isSelected(strip){
        if (!this.isLoaded() || strip == null) {
            return false;
        }
        if (DOM.hasClass(this.stripSelectionType,'single')){
            return DOM.getValue(this.selector) == strip.getName();
        } else {
            var checkbox = DOM.first(this.list,'[data-id="'+strip.getId()+'"] input[type="checkbox"]');
            return checkbox && checkbox.checked;
        }
    }

    getSelectedStrip() {
        if (DOM.hasClass(this.stripSelectionType,'single')){
            return DOM.getValue(this.select);
        }
        return null;
    }

    onComponentLoaded(component) {
        if (!this.isLoaded()) {
            return false;
        }
        var elements = component.getElements();
        var single = DOM.hasClass(elements,"single-strip");
        DOM.toggleClass(this.stripSelectionType,'single',single);
        DOM.toggleClass(this.stripSelectionType,'multiple',!single);
        DOMEvent.trigger('singleStripSelection',this.selectedValue,this);
    }

    async onAttached(elements,parent){
        this.list = DOM.first(parent,'.strip-list');
        this.rowTemplate = new HtmlTemplate(DOM.first('#strip-item'));
        assert.notNull(this.list,'unable to find .strip-list element');
        this.itemContainer = DOM.first(this.list,'.items');
        assert.notNull(this.itemContainer,'unable to find .items element');
        this.stripSelectionType = DOM.first(parent,'.strip-selection-type');
        this.select = DOM.first(parent,'#select-strip');
        this.loaded = true;
        this.loadItems();
    }

    async loadItems() {
        if (!this.isLoaded()) {
            return;
        }
        this.strips.forEach(item=>{
            const values = {
                '.name': item.name,
                '.item': new AttributeValue('data-host',item.host)
            };
            var row = this.rowTemplate.fill(values);
            DOM.setData(row,"id",item.getId());
            DOM.append(this.itemContainer,row);
        });
        var opts = this.strips.map(strip=>{return {name:strip.getName(),value:strip.getId(),dataValue:strip};});
        DOM.setOptions(this.select,opts,"--strip--");
    }
}

export default StripComponent;