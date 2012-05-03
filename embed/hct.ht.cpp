#include "autoconf.h"
#include "pinning.c"
#ifndef HEATCTL_INLINE_SUPPORT
#error Do not inline this file without HEATCTL_INLINE_SUPPORT
#endif
<html>
<head>
<title>Ethersex Heating Control Status</title>
<link rel="stylesheet" href="Sty.c" type="text/css"/>
<script src="scr.js" type="text/javascript"></script>
<script type="text/javascript">

const degParams = new Array(
	"boiler temp",
	"boiler sp",
	"outdoor temp",
	"radiator sp",
	"hotwater temp",
	"hotwater sp"
);

const stateParams = new Array(
	"burner state",
	"radiator state",
	"hotwater state",
	"circpump state"
)

const reqParams = new Array(
	"hotwater req"
)

function select_by_value(select, value) {
	var opts = select.options;
	for (var i = 0; i < opts.length; i++) {
		if (opts[i].value == value) {
			select.selectedIndex = i;
			return;
		}
	}
}

function ecmd_heatctl_set_mode(mode) {
	ArrAjax.ecmd('hc mode ' + mode, ecmd_heatctl_cmd_handler);
}

function ecmd_heatctl_req_hotw() {
	ArrAjax.ecmd('hc hotw req ' + HEATCTL_HOTWATER_REQ_INDEX, ecmd_heatctl_cmd_handler);
}

function ecmd_heatctl_cancel_hotw() {
	ArrAjax.ecmd('hc hotw req -1', ecmd_heatctl_cmd_handler);
}

#ifdef HEATCTL_CIRCPUMP_SUPPORT
function ecmd_heatctl_req_circ() {
	ArrAjax.ecmd('hc circ time', ecmd_heatctl_cmd_handler);
}

#endif
function ecmd_heatctl_cmd_handler(request) {
	ecmd_heatctl_state_req();
}

function ecmd_heatctl_state_req() {
	ArrAjax.ecmd('hc state', ecmd_heatctl_state_req_handler);
}

function ecmd_heatctl_state_req_handler(request) {
	if (ecmd_error(request))
		return;
	lines = request.responseText.split("\n");
	var hc_state_table = $('hc_state_table');

	hc_state_table.innerHTML = "";
	var row = 0;
	for (var i = 0; i < lines.length; i++) {
		if (lines[i] == "OK")
			break;
		var colums = lines[i].split(": ");
		var param = colums[0];
		var value = colums[1];
		if (param == "mode") {
			select_by_value($('hc_mode_select'), value);
			continue;
		}
		if (degParams.indexOf(param) >= 0)
			value = (parseFloat(value) / 10).toFixed(1) + " &deg;C";
		if (stateParams.indexOf(param) >= 0)
			value = parseInt(value) ? "ON" : "OFF";
		if (reqParams.indexOf(param) >= 0)
			value = parseInt(value) < 0 ? "NONE" : value;
		hc_state_table.insertRow(row++).innerHTML = "<td><code><b>" + param + "</b></code></td><td>" + value + "</td>";
	}
}

window.onload = function() {
	ecmd_heatctl_state_req();
	setInterval('ecmd_heatctl_state_req()', 10000);
}
</script>
</head>

<body>
 <h1>Heating Control Status</h1>
 <table>
  <td valign="top">
   <table id='hc_state_table' border=1 cellspacing=0>
   </table>
  </td>
  <td valign="top">
   <form>
    <table>
     <tr><td>Mode:</td></tr>
     <tr><td>
      <select id="hc_mode_select" onchange="ecmd_heatctl_set_mode(this.value)">
       <option value="manu">manual</option>
       <option value="auto">automatic</option>
       <option value="hotw">hot water</option>
       <option value="radi">radiator</option>
       <option value="serv">service</option>
      </select>
     </td></tr>
     <tr><td>Actions:</td></tr>
     <tr><td>
      <input type="button" value="Request hot water" onclick="ecmd_heatctl_req_hotw()"/>
     </td></tr>
     <tr><td>
      <input type="button" value="Cancel hot water" onclick="ecmd_heatctl_cancel_hotw()"/>
     </td></tr>
#ifdef HEATCTL_CIRCPUMP_SUPPORT
     <tr><td>
      <input type="button" value="Request circulation" onclick="ecmd_heatctl_req_circ()"/>
     </td></tr>
#endif
    </table>
   </form>
  </td>
 </table>
 <br>
 <a href="idx.ht"> Back </a>
 <div id="logconsole"></div>
</body>
</html>
