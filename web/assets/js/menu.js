function buildmenu() {
	var menu = "";
	menu += "<nav class=\"navbar navbar-expand-sm bg-dark navbar-dark fixed-top\">";
	menu +=   "<a class=\"navbar-brand\" href=\"#\">SatPI</a>";
	menu +=   "<button class=\"navbar-toggler\" type=\"button\" data-toggle=\"collapse\" data-target=\"#SatPINavbar\">";
	menu +=     "<span class=\"navbar-toggler-icon\"></span>";
	menu +=   "</button>";
	menu +=   "<div class=\"collapse navbar-collapse\" id=\"SatPINavbar\">";
	menu +=     "<ul class=\"navbar-nav\">";
	menu +=       "<li class=\"nav-item\" id=\"index\"            ><a class=\"nav-link\" href=\"index.html\"><span class=\"fas fa-home\"></span> Home</a></li>";
	menu +=       "<li class=\"nav-item\" id=\"log\"              ><a class=\"nav-link\" href=\"log.html\"><span class=\"fas fa-list-alt\"></span> Log</a></li>";
	menu +=       "<li class=\"nav-item\" id=\"frontendoverview\" ><a class=\"nav-link\" href=\"frontendoverview.html\"><span class=\"fas fa-binoculars\"></span> Frontend Overview</a></li>";
	menu +=       "<li class=\"nav-item\" id=\"frontend\"         ><a class=\"nav-link\" href=\"frontend.html\"><span class=\"fas fa-stream\"></span> Frontend</a></li>";
	menu +=       "<li class=\"nav-item\" id=\"config\"           ><a class=\"nav-link\" href=\"config.html\"><span class=\"fas fa-cogs\"></span> Configure</a></li>";
	menu +=       "<li class=\"nav-item\" id=\"contact\"          ><a class=\"nav-link\" href=\"contact.html\"><span class=\"fas fa-envelope-open\"></span> Contact</a></li>";
	menu +=       "<li class=\"nav-item\" id=\"about\"            ><a class=\"nav-link\" href=\"about.html\"><span class=\"fas fa-info-circle\"></span> About</a></li>";
	menu +=     "</ul>";
	menu +=   "</div>";
	menu += "</nav>";
	return menu;
}

function buildfooter() {
	var menu = "";
	menu += "<nav class=\"navbar navbar-expand-sm bg-dark navbar-dark fixed-bottom\">";
	menu += "  <span class=\"navbar-text\">";
	menu += "    <p class=\"\">Copyright &#169; 2014 - 2020 Marc Postema</p>";
	menu += "  </span>";
	menu += "</nav>";
	return menu;
}

function setMenuItemActive(param) {
	var obj = document.getElementById(param);
	obj.className = "active";
}
