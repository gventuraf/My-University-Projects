"use strict";

class Potion extends Item
{
    // ignore traps

    constructor(x, y, dx, dy, sprite_list, sound)
    {
        super(x, y, dx, dy, sprite_list, 0, 0, sound);
    }

    activate(player, level)
    {
        for (let i=0; i<player.traps.length; ++i) player.traps[i] = null;
        this.is_active_on_player = true;
        this.reqID = setTimeout(function(i, p) { i.remove_self(p); }, 7000, this, player);
    }

    static get_id()
    {
        return 0;
    }
}
