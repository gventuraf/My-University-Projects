"use strict";

class Trap extends Component
{
    constructor(x, y, dx, dy, sprite_list, id, timer0, timer1, sound)
    {
        super(x, y, dx, dy, sprite_list, 1, sound);
        this.id = id;
        this.reqID = -1;
        this.drawable = true;
        this.enable = false;
        setTimeout(function(o) { --o.curr_sprite; o.enabled=true; }, timer0, this);
        setTimeout(function(o) { o.enabled=o.drawable=false; }, timer1, this);
    }

    attach(player)
    {
        if (!player.isBeingPushed() && !player.isBeingStopped())
        {
            while (player.traps.length < this.id+1)
                player.traps.push(null);
            player.traps[this.id] = this;
            return true;
        }
        return false;
    }

    remove_self(player)
    {
        player.traps[this.id] = null;
    }

    action(player, level) { ; }
}
