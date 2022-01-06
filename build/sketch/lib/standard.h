#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\standard.h"
#ifndef STANDARD_VALS_H
#define STANDARD_VALS_H

namespace DevRelief {
    /*
    const char * STD_HOME = R"home(
        <html>
        <style>
        a {width: 20ch;  padding: 8px; margin: 8px; display: inline-block; margin-right: 4rem; border: 1px solid black;}
        </style>
        <body>
        <div><a>/</a>this page</div>
        <div><a>/{page}.html</a>display {page}</div>
        <div><a>/api/off</a>turn off all leds</div>
        <div><a>/api/config</a>show config.  POST to set config</div>
        <div><a>/api/std/{standard_script}</a>run "white","off","color"</div>
        <div><a>/api/script/{script}</a>return  script.  save and run with POST</div>
        <div><a>/api/run/{script}</a>run script</div>
        
        </body></home>
    )home";

    const char * STD_VARIABLES = R"vars({ 
        "RED": 0,
        "ORANGE" 30,
        "YELLOW": 60,
        "GREEN": 90,
        "CYAN": 180,
        "BLUE": 240,
        "MAGENTA": 285,
        "PURPLE": 315,
        "brightness": 40,
        "startpct": 0,
        "countpct": 100
    })vars";



    const char * STD_OFF = R"std(
        {
            "name": "std_off",
            "brightness": 0,
            "commands": [
                {
                    "type": "hsl", "component": "lightness": 0
                }
            ]
        }
    )std";

    const char * STD_WHITE = R"std(
        {
            "name": "std_white",
            "brightness": 40,
            "commands": [
                { "type": "variable", "default":true, "lightness":100, "saturation":0,"hue":0},
                {"type": "hsl", "component": "hue", "value": "var(hue)"},
                {"type": "hsl", "component": "saturation", "value": "var(saturation)"},
                {"type": "hsl", "component": "lightness", "value": "var(lightness)"}
            ]
        }
    )std";

    const char * STD_COLOR = R"std(
        {
            "name": "std_color",
            "brightness": 40,
            "commands": [
                { "type": "variable", "default":true, "lightness":50, "saturation":100,"hue":0},
                {"type": "hsl", "component": "hue", "value": "var(hue)"},
                {"type": "hsl", "component": "saturation", "value": "var(saturation)"},
                {"type": "hsl", "component": "lightness", "value": "var(lightness)"}
            ]
        }
    )std";

    const char * STD_PATTERN = R"std(
        {
            "name": "std_pattern",
            "brightness": 40,
            "commands": [
                { "type": "variable", "default":true, "lightness":50, "saturation":100,"hue":0},
                {"type": "hsl", "component": "hue", "value": "var(hue)"},
                {"type": "hsl", "component": "saturation", "value": "var(saturation)"},
                {"type": "hsl", "component": "lightness", "value": "var(lightness)"}
            ]
        }
    )std";

    const char * STD_HWEEN = R"std(
{ 
        "name": "hween",
        "brightness":40,
        "commands": [
                {
                    "type": "variable",
                    "start": 0,
                    "count": 326,
                    "startpct": 0,
                    "countpct": 100
                },
                {
                    "type": "xposition_animator",
                    "start": 0,
                    "count": "var(count)",
                    "speed": 15
                },
                {
                    "type": "hsl",
                    "component": "hue",
                    "op": "replace",
                    "start": 0,
                    "count": "var(count)",
                    "pattern": [
                        {"count": 10, "value_start":10, "value_end":20,"unfold":true}
                        ],
                    "repeat_pattern": true,
                    "position_type": "pixel",
                    "value": 0
                },    
                 {
                    "type": "hsl",
                    "component": "saturation",
                    "op": "replace",
                    "start": "var(start)",
                    "count": "var(count)",
                    "position_type": "pixel",
                    "value_start": 90,
                    "value_end": 100,
                    "duration": 3000
                },
                 {
                    "type": "hsl",
                    "component": "lightness",
                    "op": "replace",
                    "start": "var(start)",
                    "count": "var(count)",
                    "position_type": "pixel",
                    "pattern": [
                        {"count": 20, "value_start":30, "value_end":50,"unfold":true}
                        ]
                 }
                
        ]
           
    }
    )std";
    */
}

#endif