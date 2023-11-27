"use strict";

class Hourglass extends Item
{
    // more time

    constructor(x, y, dx, dy, sprite_list, sound)
    {
        super(x, y, dx, dy, sprite_list, 0, 2, sound);
    }

    activate(player, level)
    {
        //this.is_active = true;
        this.is_active_on_player = true;
        level.bottomMenu.time_left += 10; // more 10 seconds
        this.remove_self(player);
    }

    static get_id()
    {
        return 2;
    }
}
