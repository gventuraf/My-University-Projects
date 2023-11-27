"use strict";
// Weight
class SlowTrap extends Trap
{
    constructor(x, y, dx, dy, sprite_list, t0, t1, sound)
    {
        super(x, y, dx, dy, sprite_list, 3, t0, t1, sound);
    }

    attach(player)
    {
        if (!super.attach(player)) return false;
        if (player.isImune())
        { this.remove_self(player); return true; }
        player.speed = player.speed/2;
        this.reqID = setTimeout(function(obj, player)
        {
            if (!player.isBeingStopped() && !player.isBeingPushed())
                player.speed = player.true_speed;
            obj.remove_self(player);
        }, 5000, this, player);//4000
        this.sound.play();
        return true;
    }

    static get_id()
    {
        return 3;
    }
}
