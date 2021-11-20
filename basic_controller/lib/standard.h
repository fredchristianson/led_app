#ifndef STANDARD_VALS_H
#define STANDARD_VALS_H

namespace DevRelief {
    const char * STD_HOME = R"home({
        <html>
        <style>
        a { display: inline-block; margin-right: 4rem; border: 1px solid black;}
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
    })home";

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
}

#endif