"use strict";

(function() {
  window.addEventListener("load", main);
}());

function main()
{
  var music = new Audio();
  music.src = "../resources/music.wav";
  music.volume = 0.5;
  music.addEventListener("ended", (event) => {music.play();});

  var frame = document.getElementsByTagName("iframe")[0];
  window.addEventListener("message", function(ev) {
    processMessage(ev, frame, music);
  });
  frame.src = "menu.html";

  music.play();
}


function processMessage(ev, frame, music)
{
  if (ev.data.charAt(0) != "1")
  {
    if (ev.data == "exit")
      window.close();
    else
    {
        if (ev.data == 'play')
        {
            frame.width = "500";
            frame.height = "600";
        }
        else if (ev.data == 'back')
        {
            frame.width = "700";
            frame.height = "394";
        }
        frame.src = ev.data == "back" ? "menu.html" : (ev.data + ".html");
    }
  }
  else
    switch (ev.data)
    {
      case "1musicMinus": Music(music, -1, ev); break
      case "1musicPlus": Music(music, 1, ev); break;
      case "1musicOnOff": Music(music, 0, ev);
    }
}

function Music(music, v, ev)
{
  switch (v)
  {
    case 0:
            music.muted = !music.muted;
            ev.source.postMessage((music.muted?"off":"on"), '*');
    break;
    case -1:
             music.volume -= 0.1;
             if (music.volume < 0.1)
              ev.source.postMessage(0, '*');  //envia msg poe botao opaco e disabled
            if (music.volume  > 0.8)
              ev.source.postMessage(3, '*');
    break;
    case 1:
            music.volume += 0.1;
            if (music.volume > 0.9)
              ev.source.postMessage(1, '*'); //envia msg poe botao opaco e disabled
            if (music.volume > 0 && music.volume < 0.2)
              ev.source.postMessage(2, '*');
  }
}

