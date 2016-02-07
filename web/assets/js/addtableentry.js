function validateIPaddress(value) {
	// Regular Expression Pattern
	var ipformat = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
	if (value.match(ipformat)) {
		return true;
	} else {
		alert("You have entered an invalid IP address! (" + value + ")");
		return false;
	}
}

function myOnkeyPressPost(id, tagName, value, event) {
	var key = event.which;
	if (key == 13) {
		changeXMLAndPost(id, tagName, value);
	}
}

function myOnkeyPressPostIp(id, tagName, value, event) {
	var key = event.which;
	if (key == 13) {
		if (validateIPaddress(value) == true) {
			changeXMLAndPost(id, tagName, value);
		}
	}
}

function changeXMLAndPost(id, tagName, value) {
	var n = id.lastIndexOf(".");
	var sub = xmlLoaded;
	if (n != -1) {
		sub = xmlLoaded.getElementsByTagName(id.substring(0, n))[0];
	}
	var input = sub.getElementsByTagName(tagName)[0];
	input.getElementsByTagName("value")[0].childNodes[0].nodeValue = value;
//	var xmlString = (new XMLSerializer()).serializeToString(sub);
	postData(filename, xmlLoaded);
}

function addCheckboxInput(tagName, id, input) {
	var entry = "<input type='checkbox'";
	if (input.getElementsByTagName("value")[0].childNodes[0].nodeValue == "true") {
		entry += "checked=\"true\"";
	}
	entry += "name=\"" + tagName + "\"";
	entry += "id=\"" + id + "\"";
	entry += "onclick=\"changeXMLAndPost(document.getElementById(id).id, document.getElementById(id).name, document.getElementById(id).checked)\"";
	entry += "/>";
	return entry;
}

function addNumberInput(tagName, id, input) {
	var entry = "<input type=\""+ input.getElementsByTagName("inputtype")[0].childNodes[0].nodeValue + "\"";
	entry += "value=\"" + input.getElementsByTagName("value")[0].childNodes[0].nodeValue + "\"";
//	entry += "\" min=\"5\"";
	entry += "min=\"" + input.getElementsByTagName("minvalue")[0].childNodes[0].nodeValue + "\"";
	entry += "max=\"" + input.getElementsByTagName("maxvalue")[0].childNodes[0].nodeValue + "\"";

	entry += "name=\"" + tagName + "\"";
	entry += "id=\"" + id + "\"";
	entry += "onfocus=\"autoload = 0\" onblur=\"autoload = 1\"";
	entry += "onclick=\"changeXMLAndPost(document.getElementById(id).id, document.getElementById(id).name, document.getElementById(id).value)\"";
	entry += "onkeypress=\"myOnkeyPressPost(document.getElementById(id).id, document.getElementById(id).name, document.getElementById(id).value, event)\"";
	entry += "/>";
	return entry;
}

function addTextInput(tagName, id, input) {
	var entry = "<input type='text'";
	entry += "value=\"" + input.getElementsByTagName("value")[0].childNodes[0].nodeValue + "\"";
	entry += "name=\"" + tagName + "\"";
	entry += "id=\"" + id + "\"";
	entry += "onfocus=\"autoload = 0\" onblur=\"autoload = 1\"";
	entry += "onmousedown=\"document.getElementById(id).focus()\"";
	entry += "onclick=\"changeXMLAndPost(document.getElementById(id).id, document.getElementById(id).name, document.getElementById(id).value)\"";
	entry += "onkeypress=\"myOnkeyPressPost(document.getElementById(id).id, document.getElementById(id).name, document.getElementById(id).value, event)\"";
	entry += "/>";
	return entry;
}

function addIPInput(tagName, id, input) {
	var entry = "<input type='text'";
	entry += "value=\"" + input.getElementsByTagName("value")[0].childNodes[0].nodeValue + "\"";
	entry += "name=\"" + tagName + "\"";
	entry += "id=\"" + id + "\"";
	entry += "onfocus=\"autoload = 0\" onblur=\"autoload = 1\"";
	entry += "onmousedown=\"document.getElementById(id).focus()\"";
	entry += "onclick=\"changeXMLAndPost(document.getElementById(id).id, document.getElementById(id).name, document.getElementById(id).value)\"";
	entry += "onkeypress=\"myOnkeyPressPostIp(document.getElementById(id).id, document.getElementById(id).name, document.getElementById(id).value, event)\"";
	entry += "/>";
	return entry;
}

function addTableLabel(labelstring) {
	return "<td align=\"left\">" + labelstring + "</td>";
}

function addTableEntry(xmlDoc, tagName, id) {
	return addInput(xmlDoc, tagName, id);
}

function addInput(xmlDoc, tagName, id) {
	var entry = "<td align=\"left\">";
	if (xmlDoc.getElementsByTagName(tagName).length != 0) {
		var input = xmlDoc.getElementsByTagName(tagName)[0];
		var inputtype = input.getElementsByTagName("inputtype");
		if (inputtype.length != 0) {
			if (inputtype[0].childNodes[0].nodeValue == "checkbox") {
				entry += addCheckboxInput(tagName, id, input);
			} else if (inputtype[0].childNodes[0].nodeValue == "ip") {
				entry += addIPInput(tagName, id, input);
			} else if (inputtype[0].childNodes[0].nodeValue == "text") {
				entry += addTextInput(tagName, id, input);
			} else {
				entry += addNumberInput(tagName, id, input);
			}
		} else {
			entry += input.childNodes[0].nodeValue;
		}
	}
	entry += "</td>";
	return entry;
}
