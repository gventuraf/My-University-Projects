"use strict";
// Beer
class InvertTrap extends Trap
{
    constructor(x, y, dx, dy, sprite_list, t0, t1, sound)
    {
        super(x, y, dx, dy, sprite_list, 2, t0, t1, sound);
    }

    attach(player)
    {
        if (!super.attach(player)) return false;
        if (player.isImune())
        { this.remove_self(player); return true; }
        setTimeout(function(obj, player)
        {
            obj.remove_self(player);
         }, 7000, this, player);
         this.sound.play();
         return true;
    }

    action(player, level)
    {
        if (!player.isBeingStopped() && !player.isBeingPushed())
        {
            var i = 0;
            for (; i<player.directions.length && !player.directions[i]; ++i) ;
            if (i == player.directions.length) return;
            player.directions[i] = false;
            player.directions[(i+2)%player.directions.length] = true;
        }
    }

    static get_id()
    {
        return 2;
    }


}
