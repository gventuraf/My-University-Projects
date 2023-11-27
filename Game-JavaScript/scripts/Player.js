"use strict";
/* to remove
    ghost
    occupied
*/

class Player
{
    constructor(x, y, w, h, speed, image, ctx)
    {
        this.iniX = x; this.iniY = y;
        this.x = x; this.y = y;
        this.width = w; this.height = h;
        this.true_speed = speed;
        this.speed = speed;
        this.true_speed_boost = 0;
        this.speed_boost = 0;
        this.traps = new Array();
        this.items = new Array(); // NEW
        this.keys = new Array(); // NEW
        this.request_item_activation = new Array();
        //
        this.coll_matrix = [[false, false, false], [false, false, false], [false, false, false]]; // up, middle, down
        this.speeds = [speed, speed, speed, speed]; // left up right down
        //
        //this.coll_matrix = [true, true, true, true];
        this.directions = [false, false, false, false]; // left up right down
        this.pre_directions = [false, false, false, false]; // left up right down
        this.image = image;
        this.pixel_maps = this.create_pixel_maps();
        //
        this.last_direction = -1;
        this.walk_time = 0;
        this.curr_sprite = 0;
        //
        this.sx = 0;
        this.sy = 0;
    }

    draw(ctx, level)
    {
        this.set_direction();
        this.activate_items(level);

        this.do_traps(level);
        this.routine(level, false);

        // DRAW
        //ctx.drawImage(this.sprite_list[this.choose_sprite()], this.x, this.y, tileSize+8, 8+tileSize);
        ctx.fillStyle = '#000';
        //ctx.fillRect(this.x, this.y, this.width, this.height);//9, 11
        ctx.drawImage(this.image, 8+this.sx*48+4-2, 5+this.sy*48+8-2, 25+4, 30+2, this.x, this.y, this.width, this.height);
    }

    routine(level, reverse)
    {
        //this.set_direction();
        this.collisions(level.rooms[level.num_curr_room]);
        this.choose_sprite();
        this.updatePos(reverse, this.speed+this.speed_boost); // check if position is valid and update
    }

    do_traps(level)
    {
        for (let i=0; i<this.traps.length; ++i)
            if (this.traps[i])
                this.traps[i].action(this, level);
    }

    reset_keys()
    {
        this.keys.length = 0;
    }

    reset_position()
    {
        this.x = this.iniX;
        this.y = this.iniY;
    }


    choose_sprite()
    {
        let key_pressed = -1;

        for (let i=0; i<this.directions.length; ++i)
            if (this.directions[i]) key_pressed = i;

        if (key_pressed == -1) { this.last_direction = -1; return; }

        if (this.last_direction == -1)
        {
            this.curr_sprite = ++this.curr_sprite%4;
            this.walk_time = Date.now();
        }

        if (Date.now()-this.walk_time > 180)
        {
            this.curr_sprite = ++this.curr_sprite%4;
            this.walk_time = Date.now();
        }

        this.sy = this.curr_sprite;
        this.sx = ((key_pressed+1)%4);

        this.last_direction = key_pressed;
    }

    updatePos(reverse, speed)
    {
        if (!reverse)
        /*{
            if (this.directions[0] || this.directions[2])
            {
                this.x += this.directions[0] ? -speed : speed;
                return;
            }

            if (this.directions[1] || this.directions[3])
                this.y += this.directions[1] ? -speed : speed;
        }*/
        {
            if (this.directions[0] || this.directions[2])
            {
                this.x += this.directions[0] ? -this.speeds[0] : this.speeds[2];
                return;
            }

            if (this.directions[1] || this.directions[3])
                this.y += this.directions[1] ? -this.speeds[1] : this.speeds[3];
        }
        else
        {
            if (this.directions[0] || this.directions[2])
            {
                this.x += this.directions[0] ? speed : -speed;
                return;
            }

            if (this.directions[1] || this.directions[3])
                this.y += this.directions[1] ? speed : -speed;
        }
    }

    set_direction()
    {
        for (let i=0; i<this.directions.length; ++i)
            this.directions[i]=this.pre_directions[i];
    }

    activate_items(level)
    {
        let i = 0;
        for (; i<this.request_item_activation.length; ++i)
            if (this.request_item_activation[i]) break;
        if (this.items.length > i && this.items[i])
            this.items[i].activate(this, level);
        this.request_item_activation[i] = false;
    }

    clear(ctx)
    {
        ctx.clearRect(this.x, this.y, this.width, this.height);
    }

    isBeingPushed()
    {
        return this.traps.length
               && this.traps[PushTrap.get_id()];
    }

    isBeingStopped()
    {
        return this.traps.length > StopTrap.get_id()
               && this.traps[StopTrap.get_id()];
    }

    isBeingSlowed()
    {
        return this.traps.length > SlowTrap.get_id()
               && this.traps[SlowTrap.get_id()];
    }

    hasTrap(trap_id)
    {
        return this.traps.length > trap_id && this.traps[trap_id];
    }

    hasItem(item_id)
    {
        return this.items.length > item_id && this.items[item_id];
    }

    hasKey(key_id)
    {
        return this.keys.length > key_id && this.keys[key_id];
    }

    isImune()
    {
        return this.items.length > Potion.get_id()
               && this.items[Potion.get_id()]
               && this.items[Potion.get_id()].is_active_on_player;
    }

    number_of_keys()
    {
        var n = 0;
        for (let i=0; i<this.keys.length; ++i)
            if (this.keys[i]) ++n;
        return n;
    }

    // Collisions

    set_coll_matrix(room)
    {
        x_corner_tile = Math.floor((this.x + this.width/2)/tileSize) - 1; // up left corner
        y_corner_tile = Math.floor((this.y + this.height/2)/tileSize) - 1; // up left corner

        for (let lin=0; lin<this.coll_matrix.length; ++lin, ++y_corner_tile)
            for (let col=0; col<this.coll_matrix[lin].length; ++col, ++x_corner_tile)
                this.coll_matrix[lin][col] = room[y_corner_tile][x_corner_tile] > 9 &&
                                             room[y_corner_tile][x_corner_tile] < 19;
    }

    collisions(room)
    {
        //this.set_coll_matrix(room);

        // set coll_matrix

        var x_corner_tile = Math.floor((this.x + this.width/2)/tileSize) - 1; // up left corner
        var xOg = x_corner_tile;
        var y_corner_tile = Math.floor((this.y + this.height/2)/tileSize) - 1; // up left corner
        var yOg = y_corner_tile;
        var col = 0;

        for (let lin=0; lin<this.coll_matrix.length; ++lin, ++y_corner_tile)
            for (col=0, x_corner_tile=xOg; col<this.coll_matrix[lin].length; ++col, ++x_corner_tile)
                this.coll_matrix[lin][col] = (room[y_corner_tile][x_corner_tile] > 9 &&
                                             room[y_corner_tile][x_corner_tile] < 19)
                                             ||
                                             (room[y_corner_tile][x_corner_tile] > 39 &&
                                             room[y_corner_tile][x_corner_tile] < 47)
                                             ;
        //

        return this.set_speed(xOg, yOg);
    }

    set_speed(xtile, ytile)
    {
        for (let i=0; i<this.speeds.length; ++i)
            this.speeds[i] = this.speed + this.speed_boost;

        if (this.coll_matrix[1][0] && this.coll_matrix[1][2])//side hall
        {
            this.speeds[0] = this.speeds[2] = 0;
            this.directions[0] = this.directions[2] = false;
            this.pole_speed(xtile, ytile, true);
            this.pole_speed(xtile, ytile, false);
        }
        else if (this.coll_matrix[0][1] && this.coll_matrix[2][1])//pole hall
        {
            this.speeds[1] = this.speeds[3] = 0;
            this.directions[1] = this.directions[3] = false;
            this.side_speed(xtile, ytile, true);
            this.side_speed(xtile, ytile, false);
        }
        else
        {
            if (this.directions[0])
                this.side_speed(xtile, ytile, true);
            else if (this.directions[1])
                this.pole_speed(xtile, ytile, true);
            else if (this.directions[2])
                this.side_speed(xtile, ytile, false);
            else if (this.directions[3])
                this.pole_speed(xtile, ytile, false);
        }
    }

    side_speed(xtile, ytile, is_left)
    {
        var cd = is_left ? this.x - tileSize*(xtile+1) : tileSize*(xtile+2)-(this.x+this.width);

        if (this.coll_matrix[1][is_left?0:2])//center
        {
            //var y = tileSize*(ytile+1);
            //var dy = this.y + this.height - y;
            var y = this.y<tileSize*(ytile+1) ? tileSize*(ytile+1) : this.y;
            var dy = this.y<tileSize*(ytile+1) ? this.y+this.height-y : tileSize*(ytile+2)-y;
            var cd1 = this.side_pixelColl(this.x, y, this.width, dy, is_left) + cd;
            cd1 = cd1 < 1 ? 0 : cd1;
            this.speeds[is_left?0:2] = cd1 < this.speeds[is_left?0:2] ? cd1 : this.speeds[is_left?0:2];
        }

        if (this.speeds[is_left?0:2] && Math.floor((this.y+1)/tileSize) == ytile && this.coll_matrix[0][is_left?0:2])//up
        {
            var dy = tileSize*(ytile+1) - this.y;
            if (dy < 2) return;
            var cd2 = this.side_pixelColl(this.x, this.y, this.width, dy, is_left)+cd;
            cd2 = cd2 < 1 ? 0 :  cd2;
            this.speeds[is_left?0:2] = cd2 < this.speeds[is_left?0:2] ? cd2 : this.speeds[is_left?0:2];
        }

        else if (this.speeds[is_left?0:2] && Math.floor((this.y+this.height-1)/tileSize)==ytile+2 && this.coll_matrix[2][is_left?0:2])//down
        {
            var y = tileSize*(ytile+2); var dy = this.y+this.height-y;
            if (dy < 2) return;
            var cd2 = this.side_pixelColl(this.x, y, this.width, dy, is_left)+cd;
            cd2 = cd2 < 1 ? 0 :  cd2;
            this.speeds[is_left?0:2] = cd2 < this.speeds[is_left?0:2] ? cd2 : this.speeds[is_left?0:2];
        }
    }

    pole_speed(xtile, ytile, is_up)
    {
        var cd = is_up ? this.y-tileSize*(ytile+1) : tileSize*(ytile+2)-(this.y+this.height);

        if (this.coll_matrix[is_up?0:2][1])
        {
            var x = this.x<tileSize*(xtile+1) ? tileSize*(xtile+1) : this.x;
            var dx = this.x < tileSize*(xtile+1) ? this.x+this.width-x : tileSize*(xtile+2)-this.x;
            var cd1 = this.pole_pixelColl(x, this.y, dx, this.height, is_up) + cd;
            cd1 = cd1 < 1 ? 0 : cd1;
            this.speeds[is_up?1:3] = cd1 < this.speeds[is_up?1:3] ? cd1 : this.speeds[is_up?1:3];
        }

        if (this.speeds[is_up?1:3] && Math.floor((this.x+1)/tileSize)==xtile && this.coll_matrix[is_up?0:2][0])//left
        {
            var dx = tileSize*(xtile+1) - this.x;
            if (dx < 2) return;
            var cd2 = this.pole_pixelColl(this.x, this.y, dx, this.height, is_up)+cd;
            cd2 = cd2 < 0 ? 0 : cd2;
            this.speeds[is_up?1:3] = cd2 < this.speeds[is_up?1:3] ? cd2 : this.speeds[is_up?1:3];
        }

        else if (this.speeds[is_up?1:3] && Math.floor((this.x+this.width-1)/tileSize)==xtile+2 && this.coll_matrix[is_up?0:2][2])//right
        {
            var x = tileSize*(xtile+2); var dx = this.x+this.width-x;
            if (dx < 2) return;
            var cd2 = this.pole_pixelColl(x, this.y, dx, this.height, is_up)+cd;
            cd2 = cd2 < 1 ? 0 : cd2;
            this.speeds[is_up?1:3] = cd2 < this.speeds[is_up?1:3] ? cd2 : this.speeds[is_up?1:3];
        }
    }

    side_pixelColl(x, y, dx, dy, is_left)
    {
        var xPlayer = Math.ceil(x-this.x)>0?Math.ceil(x-this.x):0;
        if (!is_left) xPlayer = dx - 1;
        var yPlayer = Math.ceil(y-this.y)>0?Math.ceil(y-this.y):0;
        var originalXPlayer = xPlayer;
        var originalYPlayer = yPlayer;
        var len = dx*dy*4;

        for (let i=0; i<len; i+=4)
        {
            if (this.pixel_maps[this.sx].data[(xPlayer + yPlayer*this.width)*4 + 3] == 255)
            {
                //this.x += is_left ? dx - xPlayer - 0.1 : -xPlayer-0.1;
                return is_left ? xPlayer : dx-xPlayer;
            }

            ++yPlayer;

            if (yPlayer > originalYPlayer+dy)
            {
                yPlayer = originalYPlayer;
                xPlayer += is_left ? 1 : -1;
            }
        }

        return 0;
    }

    pole_pixelColl(x, y, dx, dy, is_up)
    {
        /**
        'x' e 'y' são absolutos
        */
        var xPlayer = Math.ceil(x-this.x)>0?Math.ceil(x-this.x):0;
        var yPlayer = Math.ceil(y-this.y)>0?Math.ceil(y-this.y):0;
        if (!is_up) yPlayer = dy - 1;
        var originalXPlayer = xPlayer;
        var originalYPlayer = yPlayer;
        var len = dx*dy*4;

        //console.log(x, y, dx, dy);
        //console.log(xPlayer, yPlayer);


        for (let i=0; i<len; i+=4)
        {
            if (this.pixel_maps[this.sx].data[(xPlayer++ + yPlayer*this.width)*4 + 3] == 255)
            {
                //this.y += is_up ? dy - yPlayer-0.1 : -yPlayer-0.1;
                return is_up ? yPlayer : dy-yPlayer;
            }

            if (xPlayer > originalXPlayer+dx)
            {
                xPlayer = originalXPlayer;
                yPlayer += is_up ? 1 : -1;
            }
        }

        return 0;
    }

    //

    create_pixel_maps()
    {
        var maps = new Array(4),
            canvas = document.createElement("canvas"),
            sx = 0, sy = 1,
            ctx = canvas.getContext("2d");

        ctx.canvas.width = this.width;
        ctx.canvas.height = this.height;


        for (; sx<4; ++sx)
        {
            ctx.drawImage(this.image, 8+sx*48+4, 5+sy*48+8, 25, 30, 0, 0, this.width, this.height);
            maps[sx] = ctx.getImageData(0, 0, ctx.canvas.width, ctx.canvas.height);
            ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
        }

        return maps;
    }


    /*


    collisions(room)
    {
        var x, y, dx, dy,
            out = true,
            co = Math.floor(this.x/tileSize), li = Math.floor(this.y/tileSize);

        for (let i=0; i<this.coll_matrix.length; ++i)
        {
            this.coll_matrix[i] = room[!i||i==1?li:li+1][!i||i==2?co:co+1]>9 &&
                                  room[!i||i==1?li:li+1][!i||i==2?co:co+1]<19;
            if (this.coll_matrix[i]) out = false;
        }

        if (out) return false;

        if (this.coll_matrix[0] && this.coll_matrix[1]) // up
        {
            x = this.x;
            y = this.y;
            dx = this.width;
            dy = tileSize * (li+1) - y;

            if (this.poleWall_pixelColl(Math.floor(x), Math.floor(y), Math.ceil(dx), Math.ceil(dy), true))
            {
                console.log('wall-up');
                return true;
            }
        }
        else if (this.coll_matrix[2] && this.coll_matrix[3]) // down
        {
            x = this.x;
            y = tileSize * (li+1);
            dx = this.width;
            dy = this.y + this.height - y;

            console.log('coll down');

            if (this.poleWall_pixelColl(Math.floor(x), Math.floor(y), Math.ceil(dx), Math.ceil(dy), false))
            {
                console.log('wall-down');
                return true;
            }
        }
        else if (this.coll_matrix[1] && this.coll_matrix[3]) // right
        {
            x = tileSize * (co+1);
            y = this.y;
            dx = this.x + this.width - x;
            dy = this.height;

            if (this.sideWall_pixelColl(Math.floor(x), Math.floor(y), Math.ceil(dx), Math.ceil(dy), false))
            {
                console.log('pixel-right');
                return true;
            }
        }
        else if (this.coll_matrix[0] && this.coll_matrix[2]) // left
        {
            x = this.x;
            y = this.y;
            dx = tileSize*(co+1) - x;
            dy = this.height;

            if (this.sideWall_pixelColl(Math.floor(x), Math.floor(y), Math.ceil(dx), Math.ceil(dy), true))
            {
                console.log('pixel-left');
                return true;
            }
        }
        else // only one is on - corner collision
        {
            let index=0;
            for (; !this.coll_matrix[index]; ++index) ;
            li += index==3 || index==2 ? 1 : 0;
            co += index==3 || index==1 ? 1 : 0;
            x = this.x < tileSize*co ? tileSize*co : this.x;
            y = this.y < tileSize*li ? tileSize*li : this.y;
            dx = this.x<tileSize*co ? this.x+this.width-x : x+tileSize-this.x;
            dy = this.y<tileSize*li ? this.y+this.height-y : y+tileSize-this.y;

            if (Math.ceil(dx) > Math.ceil(dy) &&
                this.poleWall_pixelColl(Math.floor(x), Math.floor(y), Math.ceil(dx), Math.ceil(dy), (index==2||index==3))
            )
            {
                console.log('corner-pixel-pole-coll');
                return true;
            }
            if (Math.ceil(dy) >= Math.ceil(dx) &&
                this.sideWall_pixelColl(Math.floor(x), Math.floor(y), Math.ceil(dx), Math.ceil(dy), (index==1||index==3))
            )
            {
                console.log('corner-pixel-side-coll');
                return true;
            }
        }

        return false;
    }

    sideWall_pixelColl(x, y, dx, dy, is_left)
    {
        var xPlayer = Math.ceil(x-this.x)>0?Math.ceil(x-this.x):0;
        if (!is_left) xPlayer = dx - 1;
        var yPlayer = Math.ceil(y-this.y)>0?Math.ceil(y-this.y):0;
        var originalXPlayer = xPlayer;
        var originalYPlayer = yPlayer;
        var len = dx*dy*4;

        for (let i=0; i<len; i+=4)
        {
            if (this.pixel_maps[this.sx].data[(xPlayer + yPlayer*this.width)*4 + 3] == 255)
            {
                this.x += is_left ? dx - xPlayer - 0.1 : -xPlayer-0.1;
                return true;
            }

            ++yPlayer;

            if (yPlayer > originalYPlayer+dy)
            {
                yPlayer = originalYPlayer;
                xPlayer += is_left ? 1 : -1;
            }
        }

        return false;
    }

    poleWall_pixelColl(x, y, dx, dy, is_up)
    {
        /**
        'x' e 'y' são absolutos
        *
        var xPlayer = Math.ceil(x-this.x)>0?Math.ceil(x-this.x):0;
        var yPlayer = Math.ceil(y-this.y)>0?Math.ceil(y-this.y):0;
        if (!is_up) yPlayer = dy - 1;
        var originalXPlayer = xPlayer;
        var originalYPlayer = yPlayer;
        var len = dx*dy*4;

        //console.log(x, y, dx, dy);
        //console.log(xPlayer, yPlayer);


        for (let i=0; i<len; i+=4)
        {
            if (this.pixel_maps[this.sx].data[(xPlayer++ + yPlayer*this.width)*4 + 3] == 255)
            {
                this.y += is_up ? dy - yPlayer-0.1 : -yPlayer-0.1;
                return true;
            }

            if (xPlayer > originalXPlayer+dx)
            {
                xPlayer = originalXPlayer;
                yPlayer += is_up ? 1 : -1;
            }
        }

        return false;
    }

    */

}

/*

left_speed(xtile, ytile)
{
    var cd = this.x - tileSize*(xtile+1);

    if (this.coll_matrix[1][0])//center
    {
        var y = tileSize*(ytile+1); var dy = this.y + this.height - y;
        var cd1 = this.side_pixelColl(this.x, y, this.width, dy, true) + cd;
        cd1 = cd1 < 1 ? 0 : cd1;
        this.speeds[0] = cd1 < this.speeds[0] ? cd1 : this.speeds[0];
    }

    if (this.speeds[0] && Math.floor((this.y+1)/tileSize) == ytile && this.coll_matrix[0][0])//up
    {
        var dy = tileSize*(ytile+1) - this.y;
        if (dy < 2) return;
        var cd2 = this.side_pixelColl(this.x, this.y, this.width, dy, true)+cd;
        cd2 = cd2 < 1 ? 0 :  cd2;
        this.speeds[0] = cd2 < this.speeds[0] ? cd2 : this.speeds[0];
    }

    if (this.speeds[0] && Math.floor((this.y+this.height-1)/tileSize)==ytile+2 && this.coll_matrix[2][0])//down
    {
        var y = tileSize*(ytile+2); var dy = this.y+this.height-y;
        if (dy < 2) return;
        var cd2 = this.side_pixelColl(this.x, y, this.width, dy, true)+cd;
        cd2 = cd2 < 1 ? 0 :  cd2;
        this.speeds[0] = cd2 < this.speeds[0] ? cd2 : this.speeds[0];
    }
}


*/

/*


collisions(room)
{
    var x, y, dx, dy,
        out = true,
        co = Math.floor(this.x/tileSize), li = Math.floor(this.y/tileSize);

    for (let i=0; i<this.coll_matrix.length; ++i)
    {
        this.coll_matrix[i] = room[!i||i==1?li:li+1][!i||i==2?co:co+1]>9 &&
                              room[!i||i==1?li:li+1][!i||i==2?co:co+1]<19;
        if (this.coll_matrix[i]) out = false;
    }

    if (out) return false;

    if (this.coll_matrix[0] && this.coll_matrix[1]) // up
    {
        x = this.x;
        y = this.y;
        dx = this.width;
        dy = tileSize * (li+1) - y;

        if (this.poleWall_pixelColl(Math.floor(x), Math.floor(y), Math.ceil(dx), Math.ceil(dy), true))
        {
            console.log('wall-up');
            return true;
        }
    }
    else if (this.coll_matrix[2] && this.coll_matrix[3]) // down
    {
        x = this.x;
        y = tileSize * (li+1);
        dx = this.width;
        dy = this.y + this.height - y;

        console.log('coll down');

        if (this.poleWall_pixelColl(Math.floor(x), Math.floor(y), Math.ceil(dx), Math.ceil(dy), false))
        {
            console.log('wall-down');
            return true;
        }
    }
    else if (this.coll_matrix[1] && this.coll_matrix[3]) // right
    {
        x = tileSize * (co+1);
        y = this.y;
        dx = this.x + this.width - x;
        dy = this.height;

        if (this.sideWall_pixelColl(Math.floor(x), Math.floor(y), Math.ceil(dx), Math.ceil(dy), false))
        {
            console.log('pixel-right');
            return true;
        }
    }
    else if (this.coll_matrix[0] && this.coll_matrix[2]) // left
    {
        x = this.x;
        y = this.y;
        dx = tileSize*(co+1) - x;
        dy = this.height;

        if (this.sideWall_pixelColl(Math.floor(x), Math.floor(y), Math.ceil(dx), Math.ceil(dy), true))
        {
            console.log('pixel-left');
            return true;
        }
    }
    else // only one is on - corner collision
    {
        let index=0;
        for (; !this.coll_matrix[index]; ++index) ;
        li += index==3 || index==2 ? 1 : 0;
        co += index==3 || index==1 ? 1 : 0;
        x = this.x < tileSize*co ? tileSize*co : this.x;
        y = this.y < tileSize*li ? tileSize*li : this.y;
        dx = this.x<tileSize*co ? this.x+this.width-x : x+tileSize-this.x;
        dy = this.y<tileSize*li ? this.y+this.height-y : y+tileSize-this.y;

        if (Math.ceil(dx) > Math.ceil(dy) &&
            this.poleWall_pixelColl(Math.floor(x), Math.floor(y), Math.ceil(dx), Math.ceil(dy), (index==2||index==3))
        )
        {
            console.log('corner-pixel-pole-coll');
            return true;
        }
        if (Math.ceil(dy) >= Math.ceil(dx) &&
            this.sideWall_pixelColl(Math.floor(x), Math.floor(y), Math.ceil(dx), Math.ceil(dy), (index==1||index==3))
        )
        {
            console.log('corner-pixel-side-coll');
            return true;
        }
    }

    return false;
}

sideWall_pixelColl(x, y, dx, dy, is_left)
{
    var xPlayer = Math.ceil(x-this.x)>0?Math.ceil(x-this.x):0;
    if (!is_left) xPlayer = dx - 1;
    var yPlayer = Math.ceil(y-this.y)>0?Math.ceil(y-this.y):0;
    var originalXPlayer = xPlayer;
    var originalYPlayer = yPlayer;
    var len = dx*dy*4;

    for (let i=0; i<len; i+=4)
    {
        if (this.pixel_maps[this.sx].data[(xPlayer + yPlayer*this.width)*4 + 3] == 255)
        {
            this.x += is_left ? dx - xPlayer - 0.1 : -xPlayer-0.1;
            return true;
        }

        ++yPlayer;

        if (yPlayer > originalYPlayer+dy)
        {
            yPlayer = originalYPlayer;
            xPlayer += is_left ? 1 : -1;
        }
    }

    return false;
}

poleWall_pixelColl(x, y, dx, dy, is_up)
{
    /**
    'x' e 'y' são absolutos
    *
    var xPlayer = Math.ceil(x-this.x)>0?Math.ceil(x-this.x):0;
    var yPlayer = Math.ceil(y-this.y)>0?Math.ceil(y-this.y):0;
    if (!is_up) yPlayer = dy - 1;
    var originalXPlayer = xPlayer;
    var originalYPlayer = yPlayer;
    var len = dx*dy*4;

    //console.log(x, y, dx, dy);
    //console.log(xPlayer, yPlayer);


    for (let i=0; i<len; i+=4)
    {
        if (this.pixel_maps[this.sx].data[(xPlayer++ + yPlayer*this.width)*4 + 3] == 255)
        {
            this.y += is_up ? dy - yPlayer-0.1 : -yPlayer-0.1;
            return true;
        }

        if (xPlayer > originalXPlayer+dx)
        {
            xPlayer = originalXPlayer;
            yPlayer += is_up ? 1 : -1;
        }
    }

    return false;
}



*/
