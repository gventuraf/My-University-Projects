"use strict";

class Key extends Component
{
    constructor(x, y, dx, dy, sprite_list, curr_sprite, num_room)
    {
        super(x, y, dx, dy, sprite_list, curr_sprite);
        this.id = curr_sprite;
        this.num_room = num_room;
        this.is_disabled = false;
    }

    attach(player)
    {
        if (!this.is_disabled)
        {
            while (player.keys.length < this.id+1)
                player.keys.push(null);
            player.keys[this.id] = this;

            this.is_disabled = true;
            this.curr_sprite += 3;
            return true;
        }
        return false;
    }
}
