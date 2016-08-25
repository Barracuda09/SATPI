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

function myOnkeyPressPost(tagPath, value, event) {
	var key = event.which;
	if (key == 13) {
		changeXMLAndPost(tagPath, value);
	}
}

function myOnkeyPressPostIp(tagPath, value, event) {
	var key = event.which;
	if (key == 13) {
		if (validateIPaddress(value) == true) {
			changeXMLAndPost(tagPath, value);
		}
	}
}

function changeXMLAndPost(tagPath, value) {
	var valuePath = tagPath + " value";
	xmlLoaded.querySelector(valuePath).childNodes[0].nodeValue = value
	postData(filename, xmlLoaded);
}

function addCheckboxInput(tagPath, input) {
	var entry = "<input type='checkbox'";
	if (input.getElementsByTagName("value")[0].childNodes[0].nodeValue == "true") {
		entry += "checked=\"true\"";
	}
	entry += "name=\"" + tagPath + "\"";
	entry += "id=\"" + tagPath + "\"";
	entry += "onclick=\"changeXMLAndPost(document.getElementById(id).name, document.getElementById(id).checked)\"";
	entry += "/>";
	return entry;
}

function addNumberInput(tagPath, input) {
	var entry = "<input type=\""+ input.getElementsByTagName("inputtype")[0].childNodes[0].nodeValue + "\"";
	entry += "value=\"" + input.getElementsByTagName("value")[0].childNodes[0].nodeValue + "\"";
	entry += "min=\"" + input.getElementsByTagName("minvalue")[0].childNodes[0].nodeValue + "\"";
	entry += "max=\"" + input.getElementsByTagName("maxvalue")[0].childNodes[0].nodeValue + "\"";

	entry += "name=\"" + tagPath + "\"";
	entry += "id=\"" + tagPath + "\"";
	entry += "onfocus=\"autoload = 0\" onblur=\"autoload = 1\"";
	entry += "onclick=\"changeXMLAndPost(document.getElementById(id).name, document.getElementById(id).value)\"";
	entry += "onkeypress=\"myOnkeyPressPost(document.getElementById(id).name, document.getElementById(id).value, event)\"";
	entry += "/>";
	return entry;
}

function addTextInput(tagPath, input) {
	var entry = "<input type='text'";
	entry += "value=\"" + input.getElementsByTagName("value")[0].childNodes[0].nodeValue + "\"";
	entry += "name=\"" + tagPath + "\"";
	entry += "id=\"" + tagPath + "\"";
	entry += "onfocus=\"autoload = 0\" onblur=\"autoload = 1\"";
	entry += "onmousedown=\"document.getElementById(id).focus()\"";
	entry += "onclick=\"changeXMLAndPost(document.getElementById(id).name, document.getElementById(id).value)\"";
	entry += "onkeypress=\"myOnkeyPressPost(document.getElementById(id).name, document.getElementById(id).value, event)\"";
	entry += "/>";
	return entry;
}

function addIPInput(tagPath, input) {
	var entry = "<input type='text'";
	entry += "value=\"" + input.getElementsByTagName("value")[0].childNodes[0].nodeValue + "\"";
	entry += "name=\"" + tagPath + "\"";
	entry += "id=\"" + tagPath + "\"";
	entry += "onfocus=\"autoload = 0\" onblur=\"autoload = 1\"";
	entry += "onmousedown=\"document.getElementById(id).focus()\"";
	entry += "onclick=\"changeXMLAndPost(document.getElementById(id).name, document.getElementById(id).value)\"";
	entry += "onkeypress=\"myOnkeyPressPostIp(document.getElementById(id).name, document.getElementById(id).value, event)\"";
	entry += "/>";
	return entry;
}

function addSelectionListInput(tagPath, input) {
	var list = input.getElementsByTagName("list")[0];
	var optlen = list.getElementsByTagName("*").length;
	var optsel = input.getElementsByTagName("value")[0].childNodes[0].nodeValue;

	var entry = "<select ";
	entry += "name=\"" + tagPath + "\"";
	entry += "id=\"" + tagPath + "\"";
	entry += "onfocus=\"autoload = 0\" onblur=\"autoload = 1\"";
	entry += "onmousedown=\"document.getElementById(id).focus()\"";
	entry += "onclick=\"changeXMLAndPost(document.getElementById(id).name, document.getElementById(id).value)\"";
	entry += "onkeypress=\"myOnkeyPressPost(document.getElementById(id).name, document.getElementById(id).value, event)\"";
	entry += ">";
	for (en = 0; en < optlen; en++) {
		entry += "<option";
		if (en == optsel) {
			entry += " selected=\"selected\"";
		}
		entry += ">";
		entry += list.getElementsByTagName("*")[en].childNodes[0].nodeValue;
		entry += "</option>";
	}
	entry += "</select>";
	return entry;
}

function addTableLineEntry(labelstring, xmlDoc, tagPath) {
	var page = "";
	var data = xmlDoc.querySelector(tagPath);
	if (data) {
		page += "<tr>";
		page += addTableLabel(labelstring);
		page += addTableEntry(xmlDoc, tagPath);
		page += "<tr>";
	}
	return page;
}

function addTableLabel(labelstring) {
	return "<td align=\"left\" class=\"col-md-1\">" + labelstring + "</td>";
}

function addTableEntry(xmlDoc, tagPath) {
	var entry = "<td align=\"left\" class=\"col-md-4\">";
	var input = xmlDoc.querySelector(tagPath);
	if (input) {
		var inputtype = xmlDoc.querySelector(tagPath + " inputtype");
		if (inputtype) {
			var inputTypeName = inputtype.childNodes[0].nodeValue;
			if (inputTypeName == "checkbox") {
				entry += addCheckboxInput(tagPath, input);
			} else if (inputTypeName == "ip") {
				entry += addIPInput(tagPath, input);
			} else if (inputTypeName == "text") {
				entry += addTextInput(tagPath, input);
			} else if (inputTypeName == "selectionlist") {
				entry += addSelectionListInput(tagPath, input);
			} else if (inputTypeName == "number") {
				entry += addNumberInput(tagPath, input);
			} else {
				alert("Not supported input type! (" + inputtype.childNodes[0].nodeValue + ")");
			}
		} else {
			entry += input.childNodes[0].nodeValue;
		}
	}
	entry += "</td>";
	return entry;
}
