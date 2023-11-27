"use strict";

class Level
{
    constructor()
    {
        this.rooms = new Array();
        this.portals = new Array();
        this.big_portal = null;

        this.items = new Array();
        this.keys = new Array();
        this.traps = new Array();

        this.zeros = null;
        this.num_curr_room = 0;
        this.traps_times = new Array(3); // [until_active, active, create new]
        this.traps_reqID = -1;

        this.bottomMenu = new BottomMenu();
    }

    init(ctx, images, player, sounds)
    {
        this.zeros = this.initZeros(this.rooms[this.num_curr_room]);
        this.traps.length = 0;
        this.activate_portals(player);
        this.activate_items();
        this.start_traps(images, sounds);
        this.bottomMenu.start(ctx);
    }

    stop()
    {
        this.stop_portals();
        this.stop_items();
        clearInterval(this.traps_reqID);
    }

    initZeros(room)
    {
        let zeros = new Array()
        for (let c=0; c<height; ++c)
            for (let r=0; r<width; ++r)
                if (!room[c][r])
                    zeros.push({
                        x : r * tileSize,
                        y: c * tileSize
                    });
        return zeros;
    }

    draw_room(ctx, images)
    {
        ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height-menu_height);
        /*
        ctx.fillStyle = '#000';
        ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height-menu_height);
        */
        ctx.drawImage(images.sprites_floor[0], 0, 0, ctx.canvas.width, ctx.canvas.height-menu_height);
        //
        //ctx.fillStyle = "#1653b7";
        //ctx.fillRect(0, ctx.canvas.height-menu_height, ctx.canvas.width, menu_height);
        //

        let room = this.rooms[this.num_curr_room];

        for (let c=0; c<height; ++c)
        {
            for (let r=0; r<width; ++r)
            {
                if (room[c][r] > 9 && room[c][r] < 19)
                {
                    ctx.drawImage(images.sprites_wall[room[c][r]%10], tileSize*r, tileSize*c, tileSize, tileSize);
                }
                else if (room[c][r] > 39 && room[c][r] < 47)
                {
                    ctx.drawImage(images.sprites_block[room[c][r]%10], tileSize*r, tileSize*c, tileSize, tileSize);
                }
            }
        }
    }

    //////////////////////////////////////////
    //////////  PORTALS     //////////////////

    do_portals(ctx, player)
    {
        for (let i=0; i<this.portals[this.num_curr_room].length; ++i)
        {
            this.portals[this.num_curr_room][i].draw(ctx);

            if (!player.isBeingPushed() && this.portals[this.num_curr_room][i].hasPlayerIn(player))
            {
                this.portals[this.num_curr_room][i].play_sound();
                return this.portals[this.num_curr_room][i].next_room_num * 10
                       + this.portals[this.num_curr_room][i].next_portal_num;
            }
        }

        return -3;
    }

    do_bigPortal(ctx, player)
    {
        if (!this.num_curr_room)
        {
            this.big_portal.draw(ctx);

            if (!player.isBeingPushed() && this.big_portal.hasPlayerIn(player))
            {
                if (player.number_of_keys() != this.big_portal.num_keys)
                    this.big_portal.play_sound(); // need all the keys
                else
                    return true; // change level
            }
        }

        return false;
    }

    activate_portals(player)
    {
        for (let i=0; i<this.portals[this.num_curr_room].length; ++i)
        {
            this.portals[this.num_curr_room][i].start(this.num_curr_room || !player.hasKey(i));// existe e nao e nulll
        }

        if (!this.num_curr_room)
            this.big_portal.start(true);
    }
    stop_portals()
    {
        for (let i=0; i<this.portals[this.num_curr_room].length; ++i)
            this.portals[this.num_curr_room][i].stop();
        this.big_portal.stop();
    }

    ///////////////////////////////////////////////////////////
    /////////////////   TRAPS   ///////////////////////////////

    do_traps(ctx, player)
    {
        for (let i=0; i<this.traps.length; ++i)
        {
            if (this.traps[i].drawable)
            {
                this.traps[i].draw(ctx);
                if (this.traps[i].enabled && !player.hasTrap(this.traps[i].id) &&
                    this.traps[i].hasPlayerIn(player) && this.traps[i].attach(player)
                ) this.traps.splice(i--, 1);

            }
        }
    }

    start_traps(images, sounds)
    {
        this.traps_reqID = setInterval(function(obj, images)
        {
            obj.traps.length = 0
            for (let i=0; i<obj.zeros.length; ++i)
                if (Math.random() <= 0.3)// 30% chance of happening
                {
                    let ran = Math.random(),
                        t;

                    if (ran <= 0.25)
                        t = new StopTrap(obj.zeros[i].x, obj.zeros[i].y, tileSize, tileSize, images.sprites_stopT, obj.traps_times[0], obj.traps_times[1], sounds.sound_stopT);
                    else if (ran <= 0.50)
                        t = new PushTrap(obj.zeros[i].x, obj.zeros[i].y, tileSize, tileSize, images.sprites_pushT, obj.traps_times[0], obj.traps_times[1], sounds.sound_pushT);
                    else if (ran <= 0.75)
                        t = new InvertTrap(obj.zeros[i].x, obj.zeros[i].y, tileSize, tileSize, images.sprites_InvertT, obj.traps_times[0], obj.traps_times[1], sounds.sound_invertT);
                    else
                        t = new SlowTrap(obj.zeros[i].x, obj.zeros[i].y, tileSize, tileSize, images.sprites_slowT, obj.traps_times[0], obj.traps_times[1], sounds.sound_slowT);
                    obj.traps.push(t);
                }

        }, this.traps_times[2], this, images);
    }

    ///////////////////////////////////77

    do_items(ctx, player)
    {
        for (let i=0; i<this.items[this.num_curr_room].length; ++i)
        {
            this.items[this.num_curr_room][i].draw(ctx);
            if (this.items[this.num_curr_room][i].hasPlayerIn(player) &&
                this.items[this.num_curr_room][i].attach(player, this)
            )
                ;

        }
    }

    activate_items()
    {
        for (let i=0; i<this.items[this.num_curr_room].length; ++i)
            this.items[this.num_curr_room][i].start();
    }
    stop_items()
    {
        for (let i=0; i<this.items[this.num_curr_room].length; ++i)
            this.items[this.num_curr_room][i].stop();
    }

    //

    do_keys(ctx, player)
    {
        for (let i=0; i<this.keys.length; ++i)
            if (this.keys[i].num_room == this.num_curr_room)
            {
                this.keys[i].draw(ctx);
                if (this.keys[i].hasPlayerIn(player))
                    return this.keys[i].attach(player);
            }
        return false;
    }

}
