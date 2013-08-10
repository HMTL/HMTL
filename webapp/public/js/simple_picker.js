/*
* SimplePicker.js, copyright 2012 by Contraterrene eLearning Group, LLC
* Released under the terms of the MIT License
*/

function SimplePicker(hueElementId,satvalElementId,wellElementId){
	var self = this;
	this.hueElement = document.getElementById(hueElementId);
	if(this.hueElement.offsetHeight > this.hueElement.offsetWidth){
		this.verticalHue = true;
	}
	else {
		this.verticalHue = false;
	}
	this.satvalElement = document.getElementById(satvalElementId);
	this.wellElement = document.getElementById(wellElementId);
	this.hue = 0;
	this.saturation = 0;
	this.lightness = 0;
	this.red = 0;
	this.green = 0;
	this.blue = 0;
	this.step = 0.01; // Controls gradient step size in Saturation/Value element. 
	this.RGBInput = null;
	this.HSLInput = null;
	this.redRange = null;
	this.greenRange = null;
	this.blueRange = null;
	this.hueRange = null;
	this.saturationRange = null;
	this.lightnessRange = null;
	this.bullsEyeRadius = 5;

	// Closure hack since we're going to be using some of this stuff through events.
	var me = this;

	this.setRGBInput = function(rgbInputId){
		this.RGBInput = document.getElementById(rgbInputId);
		this.updateDisplay();
	}
	this.setHSLInput = function(hslInputId){
		this.HSLInput = document.getElementById(hslInputId);
		this.updateDisplay();
	}

	this.setRGBRangeControls = function (redRangeId,greenRangeId,blueRangeId){
		this.redRange = document.getElementById(redRangeId);
		this.redRange.onchange = function(){
			me.setRedFromRangeControl();

		}
		this.greenRange = document.getElementById(greenRangeId);
		this.greenRange.onchange = function(){
			me.setGreenFromRangeControl();
		}
		this.blueRange = document.getElementById(blueRangeId);
		this.blueRange.onchange = function(){
			me.setBlueFromRangeControl();
		}
		this.updateDisplay();

	}

	this.setHSLRangeControls = function (hueRangeId,saturationRangeId,lightnessRangeId){
		this.hueRange = document.getElementById(hueRangeId);
		this.hueRange.onchange = function(){
			me.setHueFromRangeControl();
		}
		this.saturationRange = document.getElementById(saturationRangeId);
		this.saturationRange.onchange = function(){
			me.setSaturationFromRangeControl();
		}

		this.lightnessRange = document.getElementById(lightnessRangeId);
		this.lightnessRange.onchange = function(){
			me.setLightnessFromRangeControl();
		}
		this.updateDisplay();
	}
	this.getRGBString = function(){
		return this.RGBString;
	}
	this.getRGB = function(){
		return [this.red,this.green,this.blue];
	}
	this.getHSL = function(){
		return [this.hue, this.saturation, this.lightness];

	}
	this.clampRGB  = function(){ // Clamp RGB components to 0-255
		if(this.red > 255){ 
			this.red = 255;
		}
		if(this.red < 0){
			this.red = 0;
		}
		if(this.green > 255){
			this.green = 255;
		}
		if(this.green < 0){
			this.green = 0;
		}
		if(this.blue > 255){
			this.blue = 255;
		}
		if(this.blue < 0){
			this.blue = 0;
		}
	}
	this.clampHSL  = function(){ // Clamp HSL components.
		if(this.hue > 360){ // Since hue is defined as 0-360 degrees, wrap around if out of range.
			this.hue -= 360;
		}
		if(this.hue < 0){
			this.hue += 360;
		}
		if(this.saturation > 1){ // Saturation and lightness hard-clamped to between 0.0 and 1.0.
			this.saturation = 1;
		}
		if(this.saturation < 0){
			this.saturation = 0;
		}
		if(this.lightness > 1){
			this.lightness = 1;
		}
		if(this.lightness < 0){
			this.lightness = 0;
		}
	}
	this.setRGB = function(red,green,blue){ 
		this.red = red;
		this.green = green;
		this.blue = blue;
		this.clampRGB();
		this.setHSLFromRGB();
		this.updateDisplay();
	}
	this.setRed = function(red){
		this.red = red;
		this.clampRGB();
		this.setHSLFromRGB();
	}
	this.setRedFromRangeControl = function(){
		if(this.redRange == null) { 
			return;
		}
		this.setRed(this.redRange.value);
	}
	this.setGreen = function(green){
		this.green = green;
		this.clampRGB();
		this.setHSLFromRGB();
	}
	this.setGreenFromRangeControl = function(){
		if(this.greenRange == null){ 
			return;
		}
		this.setGreen(this.greenRange.value);
	}

	this.setBlue = function(blue){
		this.blue = blue;
		this.clampRGB();
		this.setHSLFromRGB();
	}

	this.setBlueFromRangeControl = function(){
		if(this.blueRange == null){ 
			return;
		}
		this.setBlue(this.blueRange.value);
	}

	this.setHSL = function (hue,saturation,lightness){
		this.hue = hue;
		this.saturation = saturation;
		this.lightness = lightness;
		this.clampHSL();
		this.setRGBFromHSL();
	}

	this.setHue = function(hue){
		this.hue = hue;
		this.setRGBFromHSL();
	}

	this.setHueFromRangeControl = function(){
		if(this.hueRange == null){ 
			return;
		}
		this.setHue(this.hueRange.value);
	}

	this.setSaturation = function(saturation){
		this.saturation = saturation;
		this.setRGBFromHSL();
	}

	this.setSaturationFromRangeControl = function(){
		if(this.saturationRange == null){
			return;
		}
		this.setSaturation(this.saturationRange.value);
	}

	this.setLightness = function(lightness){
		this.lightness = lightness;
		this.setRGBFromHSL();
	}

	this.setLightnessFromRangeControl = function(){
		if(this.lightnessRange == null){
			return;
		}
		this.setLightness(this.lightnessRange.value);
	}

	this.setBullsEyeRadius = function(newRadius){
		this.bullsEyeRadius = newRadius;
	}

	this.setHSLFromRGB = function(){
		var normalizedRed = this.red / 255.0;
		var normalizedGreen = this.green / 255.0;
		var normalizedBlue = this.blue / 255.0;
		var max = Math.max(normalizedRed,normalizedGreen,normalizedBlue);
		var min = Math.min(normalizedRed,normalizedGreen,normalizedBlue);
		var chroma  = max - min;
		this.lightness = (max + min) / 2;
		if(chroma  == 0.0){
			this.saturation = 0;
		}
		else{ 
			var hprime;
			if(max == normalizedRed){
				hprime = ((normalizedGreen - normalizedBlue) / chroma) % 6;
			}
			else if(max == normalizedGreen){
				hprime = ((normalizedBlue - normalizedRed) / chroma) + 2;
			}
			else {
				hprime = ((normalizedRed - normalizedGreen)  / chroma) + 4;
			}
			this.hue = hprime * 60;
			this.saturation = chroma / (1 - Math.abs(2 * this.lightness - 1));

		}

		this.clampHSL();
		this.updateRGBString();
		this.updateDisplay();
	}

	this.updateRGBString = function(){
		var hex = "#" + ("000000" + this.rgbToHex(this.red,this.green,this.blue)).slice(-6);
		this.RGBString = hex;
	}
	this.setRGBFromHSL = function(){
		var chroma = (1 - Math.abs(2 * this.lightness - 1)) * this.saturation;
		var hprime = this.hue / 60;
		var x = chroma * (1 - Math.abs((hprime % 2) - 1));
		var r1,g1,b1 = 0;
		if(hprime < 1){
			r1 = chroma;
			g1 = x;
			b1 = 0;
		}
		else if(hprime < 2){
			r1 = x;
			g1 = chroma;
			b1 = 0
		}
		else if(hprime < 3){
			r1 = 0;
			g1 = chroma;
			b1 = x;
		}
		else if(hprime < 4){
			r1 = 0;
			g1 = x;
			b1 = chroma;
		}
		else if(hprime < 5){
			r1 = x;
			g1 = 0;
			b1 = chroma;
		}
		else if (hprime < 6){
			r1 = chroma;
			g1 = 0;
			b1 = x;
		}
		var m = this.lightness - (0.5 * chroma);
		this.red = (r1 + m) * 255;
		this.green = (g1 + m) * 255;
		this.blue = (b1 + m) * 255;
		this.clampRGB();
		this.updateRGBString();
		this.updateDisplay();
	}

	this.calculateSVBullsEyeLocation = function(){
		this.chosenX = (this.saturation) * this.satvalElement.offsetWidth;
		this.chosenY = (this.lightness * this.satvalElement.offsetHeight);
	}

	this.updateDisplay = function(force){
		if(!force) {
			console.log('Did not happen');
			return;
		}
		this.drawSpectrum();
		this.drawSV();
		this.calculateSVBullsEyeLocation();
		this.drawBullsEyes();
		this.setWellColor();
		this.setTextInputs();
		this.setRangeControls();
	}

	this.rgbToHex = function (r, g, b) {
		if (r > 255 || g > 255 || b > 255 || r < 0  || g < 0 || b < 0)
		throw "Invalid color component";
		return ((r << 16) | (g << 8) | b).toString(16);
	}

	this.setTextInputs = function (){
		if(this.RGBInput != null){
			this.RGBInput.value = this.RGBString;
		}
		if(this.HSLInput != null){
			this.HSLInput.value = Math.round(this.hue) + "," + Math.round(this.saturation * 100) + "%," + Math.round(this.lightness * 100) + "%";
		}
	}

	this.setRangeControls = function (){
		if(this.redRange != null){
			this.redRange.value = this.red;
		}
		if(this.blueRange != null){
			this.blueRange.value = this.blue;
		}
		if(this.greenRange != null){
			this.greenRange.value = this.green;
		}
		if(this.hueRange != null){
			this.hueRange.value = this.hue;
		}
		if(this.saturationRange != null){
			this.saturationRange.value = this.saturation;
		}
		if(this.lightnessRange != null){
			this.lightnessRange.value = this.lightness;
		}
	}

	this.getSelectedColor = function(){
		var c = this.satvalElement.getContext('2d');
		this.saturation = this.chosenX / this.satvalElement.offsetWidth;
		this.lightness = this.chosenY / this.satvalElement.offsetHeight;
		this.setRGBFromHSL();
	}

	this.setWellColor = function(){

		this.wellElement.style.backgroundColor = this.RGBString;

	}

	this.drawSpectrum = function(){
		var hueContext = this.hueElement.getContext('2d');
		var hueGradient;
		if(this.verticalHue == true){
			huegradient = hueContext.createLinearGradient(this.hueElement.offsetWidth / 2,0, this.hueElement.offsetWidth / 2,this.hueElement.offsetHeight);	
		}
		else {
			huegradient = hueContext.createLinearGradient(0,this.hueElement.offsetHeight / 2,this.hueElement.offsetWidth, this.hueElement.offsetHeight / 2);
		}
		huegradient.addColorStop(0,"hsl(0, 100%, 50%)");
		huegradient.addColorStop(0.1,"hsl(36, 100%, 50%)");
		huegradient.addColorStop(0.2,"hsl(72, 100%, 50%)");
		huegradient.addColorStop(0.3,"hsl(108, 100%, 50%)");
		huegradient.addColorStop(0.4,"hsl(144, 100%, 50%)");
		huegradient.addColorStop(0.5,"hsl(180, 100%, 50%)");
		huegradient.addColorStop(0.6,"hsl(216, 100%, 50%)");
		huegradient.addColorStop(0.7,"hsl(252, 100%, 50%)");
		huegradient.addColorStop(0.8,"hsl(288, 100%, 50%)");
		huegradient.addColorStop(0.9,"hsl(324, 100%, 50%)");
		huegradient.addColorStop(1,"hsl(360, 100%, 50%)");
		hueContext.fillStyle = huegradient;
		hueContext.fillRect(0,0,this.hueElement.offsetWidth,this.hueElement.offsetHeight);

	}

	this.drawSV = function(){
		var svContext = this.satvalElement.getContext('2d');  
		var satGradient;
		var step = 1/this.satvalElement.offsetHeight;
		for(var strip = 0; strip < 1; strip+= step){
			satGradient	=  svContext.createLinearGradient(0,this.satvalElement.offsetHeight * strip,this.satvalElement.offsetWidth,this.satvalElement.offsetHeight * (strip + 0.10));
			satGradient.addColorStop(0,"hsl(" + this.hue + ",0%," + (strip * 100) + "%)");
			satGradient.addColorStop(0.5,"hsl(" + this.hue + ",50%," + (strip * 100) + "%)");
			satGradient.addColorStop(1,"hsl(" + this.hue + ",100%," + (strip * 100) + "%)");
			svContext.fillStyle = satGradient;
			svContext.fillRect(0,this.satvalElement.offsetHeight * strip, this.satvalElement.offsetWidth,this.satvalElement.offsetHeight * step);
		}
	}

	this.changeHue = function(x,y){
		this.drawSpectrum(); 
		if(this.verticalHue == true){
			this.hue = ((y / this.hueElement.offsetHeight) * 360);
		}
		else {
			this.hue = ((x / this.hueElement.offsetWidth) * 360);
		}
		this.setRGBFromHSL();

	}

	this.drawBullsEyes = function(){
		var ctx = this.satvalElement.getContext("2d");
		var myHue = this.hue;
		if(myHue < 0){
			myHue += 360;
		}
		var antiHue =  this.hue + 180;
		if(antiHue > 360){
			antiHue  = antiHue - 360;
		}
		antiHue = Math.round(antiHue);
		var antiLightness;
		if(this.lightness > 0.49){
			antiLightness = 0;
		}
		else antiLightness = 1.0;
		antiLightness = antiLightness * 100;
		var antiSaturation = this.saturation + 0.5;
		if(antiSaturation > 1.0){
			antiSaturation = antiSaturation - 1.0;
		}
		antiSaturation = antiSaturation * 100;

		var stroke =  "hsl(" + antiHue + "," + "100%," + antiLightness + "%)";
		ctx.lineWidth = 2;
		ctx.strokeStyle = stroke;
		ctx.beginPath();
		ctx.arc(this.chosenX,this.chosenY,this.bullsEyeRadius, 0, 2*Math.PI, false);
		ctx.stroke();
		ctx.closePath();

		ctx = this.hueElement.getContext("2d");

		stroke = "hsl(" + antiHue + ","  + "100%," +  "0%)"; 
		ctx.strokeStyle = stroke; 
		ctx.lineWidth = 2;
		ctx.beginPath();
		var hueX;
		var hueY;
		if(this.verticalHue == true){
			hueX  = this.hueElement.offsetWidth / 2;
			hueY = (myHue / 360) * this.hueElement.offsetHeight;
		}
		else {
			hueX  = (myHue / 360) * this.hueElement.offsetWidth;
			hueY = this.hueElement.offsetHeight / 2;
		}
		ctx.arc(hueX,hueY,this.bullsEyeRadius, 0, 2*Math.PI, false);
		ctx.stroke();
		ctx.closePath();
	}

	this.getMouseCoordinates = function(currentElement,posx,posy){
			var totalOffsetX = 0;
			var totalOffsetY = 0;
			var canvasX = 0;
			var canvasY = 0;
			
			    do{
			        totalOffsetX += currentElement.offsetLeft + currentElement.clientLeft;
			        totalOffsetY += currentElement.offsetTop + currentElement.clientTop;
			    }
			    while(currentElement = currentElement.offsetParent)
			    canvasX = posx - totalOffsetX;
			    canvasY = posy - totalOffsetY;

			    return {x:canvasX, y:canvasY}
	}
	this.hueElement.onclick =  function(e){
		var relCoords = me.getMouseCoordinates(this,e.pageX,e.pageY); // Note that "this" is the canvas in an event handler, not the SimplePicker object. :-)
		me.changeHue(relCoords.x,relCoords.y);

	};
	this.satvalElement.onclick = function(e){
		me.drawSV(); // Need to redraw to make sure we don't accidentally pick up part of the bullseye.
		if (e.pageX || e.pageY){ // From quirksmode.org. Handles difference in events between Gecko and Webkit.
			posx = e.pageX;
			posy = e.pageY;
		}
		else if (e.clientX || e.clientY) 	{
			posx = e.clientX + document.body.scrollLeft
			+ document.documentElement.scrollLeft;
			posy = e.clientY + document.body.scrollTop
			+ document.documentElement.scrollTop;
		}
	    var relCoords = me.getMouseCoordinates(this,posx,posy); // Note that "this" is the canvas in an event handler, not the SimplePicker object. :-)
		me.chosenX = relCoords.x;
		me.chosenY = relCoords.y;
		me.getSelectedColor();
	}

	this.setRGB(255,0,0);
	this.updateDisplay();
}