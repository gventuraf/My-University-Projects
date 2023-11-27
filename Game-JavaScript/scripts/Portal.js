"use strict";

class Portal extends Component
{
    constructor(x, y, dx, dy, sprite_list, next_room_num, next_portal_num, sound)
    {
        super(x, y, dx, dy, sprite_list, Math.floor(Math.random()*4), sound);
        this.next_room_num = next_room_num;
        this.next_portal_num = next_portal_num;
        this.reverse = false;
    }

    start(enabled)
    {
        this.reqID = setInterval(function(portal, enabled)
        {
            if (!enabled)
            {
                this.curr_sprite = 3;
                return;
            }
            if (portal.curr_sprite==portal.sprite_list.length-1)
                portal.reverse = true;
            else if (!portal.curr_sprite)
                portal.reverse = false;

            portal.curr_sprite += portal.reverse ? -1 : 1;

        }, 300, this, enabled);
    }
    stop()
    {
        clearInterval(this.reqID);
        this.reqID = -1;
    }

    play_sound()
    {
        this.sound.play();
    }
}
