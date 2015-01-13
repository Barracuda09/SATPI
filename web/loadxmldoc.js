function loadXMLDoc(filename) {
	refresh_time = 2000;
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else { // code for IE5 and IE6
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			xmlloaded(xmlhttp.responseXML);
		}
	}
	xmlhttp.open("GET", filename, true);
	xmlhttp.send();
}