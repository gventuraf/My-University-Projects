"use strict";

class Item extends Component
{
    constructor(x, y, dx, dy, sprite_list, curr_sprite, id, sound)
    {
        super(x, y, dx, dy, sprite_list, curr_sprite, sound);
        this.id = id;
        this.reqID = -1;
        this.is_disabled = false;
        this.is_active_on_player = false;
        this.is_active_on_board = false;
    }

    attach(player, level)
    {
        if (!this.is_disabled && !player.hasItem(this.id) && !player.isBeingPushed() && !player.isBeingStopped())
        {
            while (player.items.length < this.id+1)
            {
                player.items.push(null);
                player.request_item_activation.push(false);
            }
            player.items[this.id] = this;
            //
            this.is_disabled = true;
            this.is_active_on_board = false;
            setTimeout(function(obj) { obj.is_disabled = false; obj.is_active_on_board = true; }, 15000, this);
            return true;
        }
        return false;
    }

    remove_self(player)
    {
        player.items[this.id] = null;
        this.is_active_on_player = false;
    }

    start()
    {
        this.reqID = setInterval(function(obj)
        {
            obj.curr_sprite = obj.is_disabled ? obj.sprite_list.length-1 : ++obj.curr_sprite%(obj.sprite_list.length-1);

        }, 300, this);
    }

    stop()
    {
        clearInterval(this.reqID);
        this.reqID = -1;
        this.is_active = false;
    }
}
