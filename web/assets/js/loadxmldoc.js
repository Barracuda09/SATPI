function loadXMLDoc(filename) {
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
			xmlloaded(xmlhttp.responseXML);
		}
	}

	xmlhttp.open("GET", filename, true);
	xmlhttp.send();
}