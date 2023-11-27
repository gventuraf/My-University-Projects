"use strict";

(function() {
    window.addEventListener('load', main);
}());

const   height = 20,
        width = 20,
        tileSize = 25,
        portalWidth = tileSize,
        portalHeight = 2*tileSize,
        menu_height = 75;

function main()
{
    var canvas = document.getElementById('canvas');
    var ctx = canvas.getContext('2d');
    canvas.width = 500;
    canvas.height = 500 + menu_height;

    var images = null,
        sounds = null,
        total = 2;

    canvas.addEventListener('imagesloaded', loadedImages, {once: true});
    canvas.addEventListener('soundsloaded', loadedSounds, {once: true});

    load_images(ctx);
    load_sounds(ctx);

    function loadedImages(ev)
    {
        images = ev.images;
        if(!(--total))
            aimgLoadedHandler();
    }

    function loadedSounds(ev)
    {
        sounds = ev.sounds;
        if(!(--total))
            aimgLoadedHandler();
    }

    function aimgLoadedHandler(ev)
    {
        var player = new Player(1*tileSize, 9*tileSize, tileSize, tileSize, 1.5, images.sprites_player[0]);
        window.addEventListener('keydown', function(ev) { keysHandler(ev, player, true); });
        window.addEventListener('keyup', function(ev) { keysHandler(ev, player, false); });
        load_level(ctx, player, 1, images, sounds);
    }
}

function load_level(ctx, player, num_level, images, sounds)
{
    var level = new Level();

    ctx.canvas.addEventListener('levelinitend', astart, {once: true});

    fetch('level_'+num_level+'.txt')   //read file - 'level_'+num_level+'.txt'
        .then(response => response.text())
        .then(function(text) {
            init_level(text, level, images, ctx.canvas, sounds);
        })

    function astart(ev)
    {
        level.init(ctx, images, player, sounds);

        // MISSING : DO FADE-IN ANIMATION

        do_level(num_level, ctx, level, player, images, sounds);
    }
}

function do_level(num_level, ctx, level, player, images, sounds)
{
    var reqID = window.requestAnimationFrame(function() { do_level(num_level, ctx, level, player, images, sounds); });

    var res = render(ctx, level, player, images);

    if (res == -4)
    {
        cancelAnimationFrame(reqID);
        level.stop();
        player.reset_keys(); // retira todas as chaves do player
        player.reset_position();
        // MISSING : DO TIME-LAPSE ANIMATION
        load_level(ctx, player, num_level, images, sounds);
    }

    else if (res == - 3) return;

    else if (res == -2) // change level
    {
        cancelAnimationFrame(reqID);
        if (num_level+1 == 4) { window.parent.postMessage("back", '*'); };
        level.stop();
        player.reset_keys(); // retira todas as chaves do player
        player.reset_position();
        // MISSING : DO TIME-LAPSE ANIMATION
        load_level(ctx, player, 1+num_level, images, sounds);
    }
    else//c change room
    {
        cancelAnimationFrame(reqID);
        level.stop();
        var aux;
        aux=setInterval(function (){
            ctx.globalAlpha-=0.05;
            render(ctx,level,player,images);
            if (ctx.globalAlpha<=0.2){
                clearInterval(aux);
                if (res == -1 && level.num_curr_room) // sala principal - 0
                {
                    level.num_curr_room = 0;
                    player.reset_position();
                }
                else
                {
                    level.num_curr_room = Math.floor(res/10);
                    player.x = level.portals[level.num_curr_room][res%10].x + tileSize;
                    player.y = level.portals[level.num_curr_room][res%10].y + Math.round(tileSize/2);
                }

                level.init(ctx, images, player, sounds);
                var a=setInterval(function (){ctx.globalAlpha+=0.05;
                if(ctx.globalAlpha==1)
                {
                    clearInterval(a);
                    do_level(num_level, ctx, level, player, images, sounds);
                }
                render(ctx,level,player,images);
                },50);
            }
        },50);
    }
}

function render(ctx, level, player, images)
{
    let retv;

    /*
        retv:
            retv = sala e numPortal

        -3: mantem
        -2: muda nivel
        -4: reset level
        -1 : inicio
        sp: s e p sao numeros, sala s e portal p

        *****
        funciona desde que uma sala
        tenha sempre 10 portais ou menos
        *****

    */

    // draw background
    level.draw_room(ctx, images);
    // draw keys
    if (level.do_keys(ctx, player))
        return -1;
    // draw traps
    level.do_traps(ctx, player);
    // draw items
    level.do_items(ctx, player);
    // check portals
    retv = level.do_portals(ctx, player);
    // check big portal
    retv = level.do_bigPortal(ctx, player) ? -2 : retv;
    // draw player
    player.draw(ctx, level);
    // bottom menu update
    retv = level.bottomMenu.do(ctx, player, level, images) ? -4 : retv;

return retv;
}



//      BIG FUNCTIONS

function load_images(ctx)
{
    // MISSING CODE
    var images = new ImageContainer(),
        totImages = 66;

    //
    images.sprites_player.push(new Image());
    images.sprites_player[0].addEventListener('load', aloaded);
    images.sprites_player[0].src = 'images/george.png';

    for (let i=10; i<=18; ++i)
    {
        images.sprites_wall.push(new Image());
        images.sprites_wall[i%10].addEventListener('load', aloaded);
        images.sprites_wall[i%10].src = 'images/walls/p'+i+'.png';
    }

    for (let i=0; i<4; ++i)
    {
        images.sprites_portal.push(new Image());
        images.sprites_portal[i].addEventListener('load', aloaded);
        images.sprites_portal[i].src = 'images/portals/p'+i+'.png';
    }

    for (let i=0; i<4; ++i)
    {
        images.sprites_bigPortal.push(new Image());
        images.sprites_bigPortal[i].addEventListener('load', aloaded);
        images.sprites_bigPortal[i].src = 'images/bigPortals/p'+i+'.png';
    }

    for (let i=0, k=1; i<2; ++i, k+=4)
    {
        images.sprites_pushT.push(new Image());
        images.sprites_pushT[i].addEventListener('load', aloaded);
        images.sprites_pushT[i].src = 'images/traps/t'+k+'.png';
    }

    for (let i=0, k=2; i<2; ++i, k+=4)
    {
        images.sprites_stopT.push(new Image());
        images.sprites_stopT[i].addEventListener('load', aloaded);
        images.sprites_stopT[i].src = 'images/traps/t'+k+'.png';
    }

    for (let i=0, k=0; i<2; ++i, k+=4)
    {
        images.sprites_InvertT.push(new Image());
        images.sprites_InvertT[i].addEventListener('load', aloaded);
        images.sprites_InvertT[i].src = 'images/traps/t'+k+'.png';
    }

    for (let i=0, k=3; i<2; ++i, k+=4)
    {
        images.sprites_slowT.push(new Image());
        images.sprites_slowT[i].addEventListener('load', aloaded);
        images.sprites_slowT[i].src = 'images/traps/t'+k+'.png';
    }

    for (let i=0; i<6; ++i)
    {
        images.sprites_keys.push(new Image());
        images.sprites_keys[i].addEventListener('load', aloaded);
        images.sprites_keys[i].src = 'images/keys/key'+i+'.png';
    }

    for (let i=0; i<10; ++i)
    {
        images.sprites_timer.push(new Image());
        images.sprites_timer[i].addEventListener('load', aloaded);
        images.sprites_timer[i].src = 'images/numbers/timer/digit'+i+'.png';
    }

    // 40 images this far

    images.sprites_timer.push(new Image());
    images.sprites_timer[10].addEventListener('load', aloaded);
    images.sprites_timer[10].src = 'images/numbers/timer/colon.png';

    images.sprites_numbers.push(new Image());
    images.sprites_numbers[0].addEventListener('load', aloaded);
    images.sprites_numbers[0].src = 'images/numbers/numbers.png';

    //

    for (let i=0; i<4; ++i)
    {
        images.sprites_potionI.push(new Image());
        images.sprites_potionI[i].addEventListener('load', aloaded);
        images.sprites_potionI[i].src = 'images/items/potion'+i+'.png';
    }

    for (let i=0; i<4; ++i)
    {
        images.sprites_ghostI.push(new Image());
        images.sprites_ghostI[i].addEventListener('load', aloaded);
        images.sprites_ghostI[i].src = 'images/items/ghost'+i+'.png';
    }

    for (let i=0; i<4; ++i)
    {
        images.sprites_hourglassI.push(new Image());
        images.sprites_hourglassI[i].addEventListener('load', aloaded);
        images.sprites_hourglassI[i].src = 'images/items/hourglass'+i+'.png';
    }

    for (let i=0; i<4; ++i)
    {
        images.sprites_coffeeI.push(new Image());
        images.sprites_coffeeI[i].addEventListener('load', aloaded);
        images.sprites_coffeeI[i].src = 'images/items/coffee'+i+'.png';
    }

    for (let i=0; i<7; ++i)
    {
        images.sprites_block.push(new Image());
        images.sprites_block[i].addEventListener('load', aloaded);
        images.sprites_block[i].src = 'images/blocks/b'+i+'.png';
    }

    images.sprites_floor.push(new Image());
    images.sprites_floor[0].addEventListener('load', aloaded);
    images.sprites_floor[0].src = 'images/floor.png';

    function aloaded()
    {
        if (!(--totImages))
        {
            var myev = new Event('imagesloaded');
            myev.images = images;
            ctx.canvas.dispatchEvent(myev);
        }
    }
}

function load_sounds(ctx)
{
    var sounds= new SoundContainer(),
        totSounds=10;

    sounds.sound_portal = new Audio();
    sounds.sound_portal.addEventListener('loadeddata',soundsloaded);
    sounds.sound_portal.src= 'sounds/teleport.wav';

    sounds.sound_pushT = new Audio();
    sounds.sound_pushT.addEventListener('loadeddata',soundsloaded);
    sounds.sound_pushT.src= 'sounds/hit.ogg';

    sounds.sound_stopT = new Audio();
    sounds.sound_stopT.addEventListener('loadeddata',soundsloaded);
    sounds.sound_stopT.src= 'sounds/controller.ogg';

    sounds.sound_slowT = new Audio();
    sounds.sound_slowT.addEventListener('loadeddata',soundsloaded);
    sounds.sound_slowT.src= 'sounds/slow.mp3';

    sounds.sound_invertT = new Audio();
    sounds.sound_invertT.addEventListener('loadeddata',soundsloaded);
    sounds.sound_invertT.src= 'sounds/cerveja.wav';

    sounds.sound_hourglass = new Audio ();
    sounds.sound_hourglass.addEventListener('loadeddata',soundsloaded);
    sounds.sound_hourglass.src= 'sounds/time_stop.mp3';

    sounds.sound_ghost = new Audio();
    sounds.sound_ghost.addEventListener('loadeddata',soundsloaded);
    sounds.sound_ghost.src= 'sounds/ghostbreath.flac';

    sounds.sound_coffee = new Audio();
    sounds.sound_coffee.addEventListener('loadeddata',soundsloaded);
    sounds.sound_coffee.src= 'sounds/cafe.wav';

    sounds.sound_potion = new Audio();
    sounds.sound_potion.addEventListener('loadeddata',soundsloaded);
    sounds.sound_potion.src= 'sounds/pocao.ogg';

    sounds.sound_bigPortal= new Audio();
    sounds.sound_bigPortal.addEventListener('loadeddata',soundsloaded);
    sounds.sound_bigPortal.src= 'sounds/reject.mp3';


    function soundsloaded()
    {
        if (!(--totSounds))
        {
            var myev = new Event('soundsloaded');
            myev.sounds = sounds;
            ctx.canvas.dispatchEvent(myev);
        }
    }
}

function init_level(text, level, images, canvas, sounds)
{
    var lines = text.split('\n');
    --lines.length;//last element is empty, remove it
    let r = 0,
        line_counter = 0,
        num_rooms = parseInt(lines.shift());

    // allocate memory for rooms
    for (let i=0; i<num_rooms; ++i)
    {
        var tmp_room = new Array(height);
        for (let j=0; j<height; ++j)
            tmp_room[j] = new Array(width);
        level.rooms.push(tmp_room);
    }

    // allocate memory for portals
    for (let i=0; i<num_rooms; ++i)
        level.portals.push(new Array());

    // allocate memory for items
    for (let i=0; i<num_rooms; ++i)
        level.items.push(new Array());

    // fill rooms and portals
    for (let i=0; r<num_rooms;)
    {
        let line = lines[line_counter++].split(',');
        line = line.map(Number);

        for (let j=0; j<line.length; ++j)
        {
            level.rooms[r][i][j] = line[j];
            switch (line[j])
            {
                case 2: // portal
                        level.portals[r].push(new Portal(j*tileSize, i*tileSize, portalWidth, portalHeight, images.sprites_portal,0,0, sounds.sound_portal));
                        break;
                case 30: case 31: case 32: // keys
                        level.keys.push(new Key(j*tileSize, i*tileSize, tileSize, tileSize, images.sprites_keys, line[j]%10, r));
                        break;
                case 5: // big portal
                        level.big_portal = new BigPortal(j*tileSize, i*tileSize, 2*tileSize, 2*tileSize, images.sprites_bigPortal, sounds.sound_bigPortal);
                        break;
                case 60: //item potion
                        level.items[r].push(new Potion(j*tileSize, i*tileSize, tileSize, tileSize, images.sprites_potionI, sounds.sounds_potion));
                        break;
                case 61: // item ghost
                        level.items[r].push(new Ghost(j*tileSize, i*tileSize, tileSize, tileSize, images.sprites_ghostI, sounds.sounds_ghost));
                        break;
                case 62: // item hourglass
                        level.items[r].push(new Hourglass(j*tileSize, i*tileSize, tileSize, tileSize, images.sprites_hourglassI, sounds.sounds_hourglass));
                        break;
                case 63: // item coffee
                        level.items[r].push(new Coffee(j*tileSize, i*tileSize, tileSize, tileSize, images.sprites_coffeeI, sounds.sounds_coffee));
            }
        }

        if (!(++i % height))// new room
        {
            ++r;
            i = 0;
        }
    }

    // finish initializing portals
    let line = lines[line_counter++].split(',');
    line = line.map(Number);
    for (let i=0, k=0; i<level.portals.length; ++i)
        for (let j=0; j<level.portals[i].length; ++j)
        {
            level.portals[i][j].next_room_num = line[k++];
            level.portals[i][j].next_portal_num = line[k++];
        }

    level.big_portal.num_keys = level.portals[0].length;

    // read & assign:
    //          BottomMenu images,
    //          BottomMenu.currentDate,
    //          BottomMenu.time_left,
    //          trap's time
    level.bottomMenu.sprites_timer = images.sprites_timer;
    level.bottomMenu.sprites_numbers = images.sprites_numbers;
    //level.bottomMenu.currentDate = Number(lines[line_counter++]);
    level.bottomMenu.time_left = Number(lines[line_counter++]) * num_rooms;
    line = lines[line_counter++].split(',');
    line = line.map(Number);
    for (let i=0; i<level.traps_times.length; ++i)
        level.traps_times[i] = line[i];

    var myev = new Event('levelinitend');
    canvas.dispatchEvent(myev);
}


function keysHandler(ev, player, on)
{//left 37; up 38; right 39; down 40
    switch (ev.keyCode)
    {
        case 37: case 65:
                player.pre_directions[0] = on;
                //player.pre_directions[2] = !on;
                if (on)
                    player.pre_directions[1] = player.pre_directions[2] = player.pre_directions[3] = false;
                break;
        case 38: case 87:
                player.pre_directions[1] = on;
                //player.pre_directions[3] = !on;
                if (on)
                    player.pre_directions[0] = player.pre_directions[2] = player.pre_directions[3] = false;
                break;
        case 39: case 68:
                player.pre_directions[2] = on;
                //player.pre_directions[0] = !on;
                if (on)
                    player.pre_directions[0] = player.pre_directions[1] = player.pre_directions[3] = false;
                break;
        case 40: case 83:
                player.pre_directions[3] = on;
                //player.pre_directions[1] = !on;
                if (on)
                {
                    player.pre_directions[0] = player.pre_directions[1] = player.pre_directions[2] = false;
                }
        case 49:
                player.request_item_activation[0] = true;
                break;
        case 50:
                player.request_item_activation[1] = true;
                break;
        case 51:
                player.request_item_activation[2] = true;
                break;
        case 52:
                player.request_item_activation[3] = true;
                break;
        case 27:
                window.parent.postMessage("back", '*');
    }
}
