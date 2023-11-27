"use strict";

class Ghost extends Item
{
    // ignore collisions

    constructor(x, y, dx, dy, sprite_list, sound)
    {
        super(x, y, dx, dy, sprite_list, 0, 1, sound);
    }

    attach(player, level)
    {
        if (!super.attach(player)) return false;
    }

    activate(player, level)
    {
        this.reqID = setTimeout(function(i, p) { i.remove_self(p); }, 7000, this, player);
        this.is_active_on_player = true;
        //console.log('ghosting');
    }

    static get_id()
    {
        return 1;
    }
}
