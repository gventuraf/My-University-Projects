"use strict";

(function() {
  window.addEventListener("load", main);
}());

function main()
{
  var buttons = document.getElementsByTagName("nav")[0];
  buttons.addEventListener("click", buttonsHandler);
}

function buttonsHandler(ev)
{
    var msg = buttonIdToAction(ev.target.id);
    if (msg != "nav")
      window.parent.postMessage(msg, '*');
    ev.stopPropagation();
}

function buttonIdToAction(buttonId)
{
  var i;
  for (i=0; buttonId.charAt(i) >= 'a' && i<buttonId.length; ++i)
    ;
  return buttonId.substring(0, i);
}
