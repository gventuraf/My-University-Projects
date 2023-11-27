"use strict";

class Coffee extends Item
{
    // increase speed

    constructor(x, y, dx, dy, sprite_list, sound)
    {
        super(x, y, dx, dy, sprite_list, 0, 3, sound);
    }

    activate(player, level)
    {
        player.speed_boost = player.true_speed_boost = player.true_speed;
        this.is_active_on_player = true;
        this.reqID = setTimeout(function(i, p)
        {
            p.speed_boost = p.true_speed_boost = 0;
            i.remove_self(p);
        }, 5000, this, player);
    }

    static get_id()
    {
        return 3;
    }

    draw(ctx)
    {
        ctx.drawImage(this.sprite_list[this.curr_sprite], this.x-5, this.y-5, this.width+10, 10+this.height);
    }
}
