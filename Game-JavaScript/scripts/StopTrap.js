"use strict";
// Controller
class StopTrap extends Trap
{
    constructor(x, y, dx, dy, sprite_list, t0, t1, sound)
    {
        super(x, y, dx, dy, sprite_list, 1, t0, t1, sound);
    }

    attach(player)
    {
        if (!super.attach(player)) return false;
        if (player.isImune())
        { this.remove_self(player); return true; }
        player.speed = player.speed_boost = 0;
        setTimeout(function(obj, player)
        {
            if (!player.isBeingPushed())
                player.speed = player.true_speed/(player.isBeingSlowed() ? 2 : 1);
            player.speed_boost = player.hasItem(Coffee.get_id()) ? player.true_speed : 0;;
            obj.remove_self(player);
        }, 2000, this, player);
        this.sound.play();
        return true;
    }

    static get_id()
    {
        return 1;
    }
}
