"use strict";

class BigPortal extends Portal
{
    constructor(x, y, dx, dy, sprite_list, sound)//next_room_num, next_portal_num
    {
        super(x, y, dx, dy, sprite_list, -1, 0, sound);
        this.num_keys = 0;
        this.audio_playing = false;
    }

    stop()
    {
        if (this.reqID != -1)
            super.stop();
    }

    play_sound()
    {
        if (!this.audio_playing)
        {
            this.sound.play();
            this.audio_playing = true;
            setTimeout(function(obj) { obj.audio_playing = false; }, 5000, this);
        }
    }
}
