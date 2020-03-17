function buildmenu() {
// Glyphicons: see http://glyphicons.com/
	var menu = "";
	menu += "<nav class=\"navbar navbar-inverse navbar-fixed-top\">";
	menu +=   "<div class=\"container\">";
	menu +=     "<div class=\"navbar-header\">";
	menu +=       "<button type=\"button\" class=\"navbar-toggle\" data-toggle=\"collapse\" data-target=\"#satpinavbar\">";
	menu +=         "<span class=\"sr-only\">Toggle navigation</span>";
	menu +=         "<span class=\"icon-bar\"></span>";
	menu +=         "<span class=\"icon-bar\"></span>";
	menu +=         "<span class=\"icon-bar\"></span>";
	menu +=       "</button>";
	menu +=       "<a class=\"navbar-brand\" href=\"#\">SAT>IP</a>";
	menu +=     "</div>";
	menu +=     "<div class=\"collapse navbar-collapse\" id=\"satpinavbar\">";
	menu +=       "<ul class=\"nav navbar-nav\">";
	menu +=         "<li id=\"index\"            class=\"\"><a href=\"index.html\"><span class=\"glyphicon glyphicon-home\"></span> Home</a></li>";
	menu +=         "<li id=\"log\"              class=\"\"><a href=\"log.html\"><span class=\"glyphicon glyphicon-list\"></span> Log</a></li>";
	menu +=         "<li id=\"frontendoverview\" class=\"\"><a href=\"frontendoverview.html\"><span class=\"glyphicon glyphicon-info-sign\"></span> Frontend Overview</a></li>";
	menu +=         "<li id=\"frontend\"         class=\"\"><a href=\"frontend.html\"><span class=\"glyphicon glyphicon-info-sign\"></span> Frontend</a></li>";
	menu +=         "<li id=\"config\"           class=\"\"><a href=\"config.html\"><span class=\"glyphicon glyphicon-cog\"></span> Configure</a></li>";
	menu +=         "<li id=\"contact\"          class=\"\"><a href=\"contact.html\"><span class=\"glyphicon glyphicon-envelope\"></span> Contact</a></li>";
	menu +=         "<li id=\"about\"            class=\"\"><a href=\"about.html\"><span class=\"glyphicon glyphicon-book\"></span> About</a></li>";
	menu +=       "</ul>";
	menu +=       "<ul class=\"nav navbar-nav navbar-right\">";
	menu +=         "<li id=\"stop\" class=\"\"><a href=\"STOP\"><span class=\"glyphicon glyphicon-off\"></span> Stop</a></li>";
	menu +=       "</ul>";
	menu +=     "</div>";
	menu +=   "</div>";
	menu += "</nav><!-- nav end -->";
	return menu;
}

function buildfooter() {
	var menu = "";
	menu += "<div class=\"navbar navbar-default navbar-fixed-bottom\">";
	menu += "	<div class=\"container\">";
	menu += "		<p class=\"navbar-text pull-left\">Copyright &#169; 2014 - 2020 Marc Postema</p>";
	menu += "	</div>";
	menu += "</div>";
	return menu;
}

function setMenuItemActive(param) {
	var obj = document.getElementById(param);
	obj.className = "active";
}
