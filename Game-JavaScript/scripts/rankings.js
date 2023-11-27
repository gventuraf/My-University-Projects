"use strict";

(function() {
  window.addEventListener("load", main);
}());

function main()
{
  var backButton = document.getElementsByTagName("button")[0];
  backButton.addEventListener("click", (event) =>
  {
    window.parent.postMessage("back", '*');
  });
}
