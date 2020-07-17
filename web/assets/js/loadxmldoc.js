var xmlLoaded;
var filename;
function loadXMLDoc(file) {
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else if (window.ActiveXObject) {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	} else {
		throw new Error("Ajax is not supported by this browser");
	}

	// callback function
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			filename = file;
			xmlLoaded = xmlhttp.responseXML;
			xmlloaded(xmlhttp.responseXML);
		}
	}

	xmlhttp.open("GET", file, true);
	xmlhttp.send();
}

function loadJSONDoc(filename) {
	if (window.XMLHttpRequest) {
		jsonhttp = new XMLHttpRequest();
	} else if (window.ActiveXObject) {
		jsonhttp = new ActiveXObject("Microsoft.XMLHTTP");
	} else {
		throw new Error("Ajax is not supported by this browser");
	}

	// callback function
	jsonhttp.onreadystatechange = function() {
		if (jsonhttp.readyState == 4 && jsonhttp.status == 200) {
			jsonloaded(jsonhttp.responseText);
		}
	}

	jsonhttp.open("GET", filename, true);
	jsonhttp.send();
}
