"use strict";

(function() {
  window.addEventListener("load", main);
}());

function main()
{
  var backButton = document.getElementById("backButton");
  var nav = document.getElementsByTagName("nav")[0];

  window.addEventListener("message", processMessage);

  backButton.addEventListener("click", buttonsHandler);
  nav.addEventListener("click", buttonsHandler);
  //navs[1].addEventListener("click", buttonsHandler);
}

function buttonsHandler(ev)
{
  if (ev.target.id == "backButton")
    window.parent.postMessage("back", '*');
  else if (ev.target.id != "")
    window.parent.postMessage("1" + ev.target.id, '*');
}

function processMessage(ev)
{
  if (ev.data === "on" || ev.data === "off")
  {
    var musicOnOff = document.getElementById("musicOnOff");
    musicOnOff.style.backgroundImage = ev.data==="on" ? "url('../resources/musicOn.svg')" : "url('../resources/musicOff.svg')";
    console.log("HERE");
  }
  else if (ev.data===2 || ev.data===3)
  {
    var btn = document.getElementById((ev.data===2?"musicMinus":"musicPlus"));
    btn.disabled = false;
    btn.style.opacity = 1.0;
  }
  else if (ev.data==0 || ev.data==1)
  {
    var btn = document.getElementById((ev.data===1?"musicPlus":"musicMinus"));
    btn.disabled = true;
    btn.style.opacity = 0.3;
  }
}
