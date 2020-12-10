function setCookie(cookieName, cookieValue, expireDays) {
	var date = new Date();
	date.setTime(date.getTime() + (expireDays * 24 * 3600 * 1000));
	var expireString = "expires=" + date.toUTCString();
	document.cookie = cookieName + "=" + cookieValue + ";" + expireString + ";path=/";
}

function getCookie(cookieName) {
	var searchCookieTag = cookieName + "=";
	var decodedCookie = decodeURIComponent(document.cookie);
	var cookieTagArray = decodedCookie.split(";");
	for(var i = 0; i < cookieTagArray.length; ++i) {
		var cookieTag = cookieTagArray[i];
		while (cookieTag.charAt(0) == ' ') {
			cookieTag = cookieTag.substring(1);
		}
		if (cookieTag.indexOf(searchCookieTag) == 0) {
			return cookieTag.substring(searchCookieTag.length, cookieTag.length);
		}
	}
	return "";
}
