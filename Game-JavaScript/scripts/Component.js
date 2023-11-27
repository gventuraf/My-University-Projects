"use strict";

class Component
{
    constructor(x, y, dx, dy, sprite_list, curr_sprite, sound)
    {
        this.x = x; this.y = y;
        this.width = dx;
        this.height = dy;
        this.sprite_list = sprite_list;
        this.curr_sprite = curr_sprite;
        this.reqID = -1;
        this.sound = sound;
    }

    draw(ctx)
    {
        ctx.drawImage(this.sprite_list[this.curr_sprite], this.x, this.y, this.width, this.height);
    }

    hasPlayerIn(player)
    {
        let playerX = Math.round(player.x + player.width/2),
            playerY = Math.round(player.y + player.height/2);
        return !(playerX < this.x || playerX > this.x+this.width
                || playerY < this.y || playerY > this.y+this.height);
    }

}


/*


hasPlayerIn(player)
{
    if ((player.x+player.width < this.x || player.x > this.x+this.width
        || player.y+player.height < this.y || player.y > this.y+this.height)
    )
        return false; // no collision

    var res = this.pixel_coll(player.x < this.x ? this.x : player.x,
                           player.y < this.y ? this.y : player.y,
                           player.x+player.width < this.x+this.width ? player.x+player.width-this.x : this.x+this.width-player.x,
                           player.y+player.height < this.y+this.height ? player.y+player.height-this.y : this.y+this.height-player.y,
                           player);
    console.log(player.x < this.x ? this.x : player.x,
                           player.y < this.y ? this.y : player.y,
                           player.x+player.width < this.x+this.width ? player.x+player.width-this.x : this.x+this.width-player.x,
                           player.y+player.height < this.y+this.height ? player.y+player.height-this.y : this.y+this.height-player.y,
                           player);
    return res;

    return this.pixel_coll(player.x < this.x ? this.x : player.x,
                           player.y < this.y ? this.y : player.y,
                           player.x+player.width < this.x+this.width ? player.x+player.width-this.x : this.x+this.width-player.x,
                           player.y+player.height < this.y+this.height ? player.y+player.height-this.y : this.y+this.height-player.y,
                           player);
}

pixel_coll(x, y, dx, dy, player)
{
    //
    //'x' e 'y' são absolutos
    //
    var xComponent = Math.ceil(x-this.x); var yComponent = Math.ceil(y-this.y);
    var xPlayer = Math.ceil(x-player.x); var yPlayer = Math.ceil(y-player.y);
    var originalXComponent = xComponent; var originalXPlayer = xPlayer;
    var len = dx*dy*4;

    for (let i=0; i<len; i+=4)
    {
        if (this.pixel_map.data[(xComponent++ + yComponent*this.width)*4 + 3] > 200
            &&
            player.pixel_maps[player.sx].data[(xPlayer++ + yPlayer*player.width)*4 + 3] > 200
        )
            return true;

            if (xComponent > originalXComponent+dx)
            {
                xComponent = originalXComponent;
                ++yComponent;
            }
            if (xPlayer > originalXPlayer)
            {
                xPlayer = originalXPlayer;
                ++yPlayer;
            }
    }

    return false;
}

create_pixel_map(x, y, dx, dy)
{
    var canvas = document.createElement("canvas"),
        ctx = canvas.getContext("2d");

    ctx.canvas.width = this.width;
    ctx.canvas.height = this.height;


    ctx.drawImage(this.sprite_list[this.curr_sprite], 0+(x?x:0), 0+(y?y:0), this.width+(dx?dx:0), this.height+(dy?dy:0));
    return ctx.getImageData(0, 0, ctx.canvas.width, ctx.canvas.height);
}


*/
