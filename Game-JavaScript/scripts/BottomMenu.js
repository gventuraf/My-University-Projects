"use strict";

class BottomMenu
{
    constructor()
    {
        /*
        this.date_image = date_image;
        this.slash = slash;
        this.colon = colon;
        this.timer_images = timer_images;
        this.items_timer_images = items_timer_images;
        this.traps_timer_images = traps_timer_images;
        */
        this.x = 0;
        this.y = 0; // initialized in this.start()
        this.width = 0; // initialized in this.start()
        this.height = menu_height;
        this.sprites_timer = null;
        this.sprites_numbers = null;
        this.startTime = 0;
        this.time_left = 0; // filled in init_level()
        this.currentDate = 0; // filled in init_level()
        this.reqID = -1;
    }

    start(ctx)
    {
        this.y = ctx.canvas.height - menu_height;
        this.width = ctx.canvas.width;
        this.startTime = Date.now();
    }

    do(ctx, player, level, images)
    // vai precisar do level para saber os items e traps
    // e keys/gemas que ha neste nivel
    {
        // background
        ctx.clearRect(this.x, this.y, this.width, this.height);
        ctx.fillStyle = '#1653b7';
        ctx.fillRect(this.x, this.y, this.width, this.height);
            // date
        ;
        //

        if (this.timer(ctx))
            return true; // time ended => level over

        this.items(ctx, player, images);
        this.traps(ctx, player, images);
        this.keys(ctx, player, images);

        return false; // timer is not over
    }

    keys(ctx, player, images)
    {
        var x = 0;

        var image = images.sprites_keys[player.hasKey(x) ? x : x+3];
        ctx.drawImage(image, 425, this.y+5, tileSize, tileSize);
        ++x;

        image = images.sprites_keys[player.hasKey(x) ? x : x+3];
        ctx.drawImage(image, 450, this.y+tileSize, tileSize, tileSize);
        ++x;

        image = images.sprites_keys[player.hasKey(x) ? x : x+3];
        ctx.drawImage(image, 425, this.y+2*tileSize, tileSize, tileSize);
        //++x;
    }

    traps(ctx, player, images)
    {
        var x = 0;

        var image = images.sprites_pushT[player.hasTrap(x) ? 0 : images.sprites_pushT.length-1];
        ctx.drawImage(image, 100+(x++*75), 40+this.y, tileSize, tileSize);

        image = images.sprites_stopT[player.hasTrap(x) ? 0 : images.sprites_stopT.length-1];
        ctx.drawImage(image, 100+(x++*75), 40+this.y, tileSize, tileSize);

        image = images.sprites_InvertT[player.hasTrap(x) ? 0 : images.sprites_InvertT.length-1];
        ctx.drawImage(image, 100+(x++*75), 40+this.y, tileSize, tileSize);

        image = images.sprites_slowT[player.hasTrap(x) ? 0 : images.sprites_slowT.length-1];
        ctx.drawImage(image, 100+(x++*75), 40+this.y, tileSize, tileSize);
    }

    items(ctx, player, images)
    {
        var x = 0;

        var image = images.sprites_potionI[player.hasItem(x) ? 0 : images.sprites_potionI.length-1];
        ctx.drawImage(image, 100+(x++*75), 5+this.y, tileSize, tileSize);

        image = images.sprites_ghostI[player.hasItem(x) ? 0 : images.sprites_ghostI.length-1];
        ctx.drawImage(image, 100+(x++*75), 5+this.y, tileSize, tileSize);

        image = images.sprites_hourglassI[player.hasItem(x) ? 0 : images.sprites_hourglassI.length-1];
        ctx.drawImage(image, 100+(x++*75), 5+this.y, tileSize, tileSize);

        image = images.sprites_coffeeI[player.hasItem(x) ? 0 : images.sprites_coffeeI.length-1];
        ctx.drawImage(image, 100+(x++*75)-5, 5+this.y-5, tileSize+10, tileSize+10);
    }

    timer(ctx)
    {
        if (this.reqID < 0)
            this.reqID = setInterval(function(o) { --o.time_left; }, 1000, this);

        if (this.time_left < 0)
        {
            clearInterval(this.reqID);
            this.time_left = 0;
        }

        var sec = this.time_left%60,
            min = Math.floor(this.time_left/60);

        ctx.drawImage(this.sprites_timer[Math.floor(min/10)], 0+3, this.y+21, 19, 33);
        ctx.drawImage(this.sprites_timer[min%10], 19+3, this.y+21, 19, 33);
        ctx.drawImage(this.sprites_timer[10], 38+3, this.y+21, 11, 33);
        ctx.drawImage(this.sprites_timer[Math.floor(sec/10)], 49+3, this.y+21, 19, 33);
        ctx.drawImage(this.sprites_timer[sec%10], 68+3, this.y+21, 19, 33);

        return this.time_left <= 0;
    }
}
