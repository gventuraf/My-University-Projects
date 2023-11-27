"use strict";
// Punch
class PushTrap extends Trap
{
    constructor(x, y, dx, dy, sprite_list, t0, t1, sound)
    {
        super(x, y, dx, dy, sprite_list, 0, t0, t1, sound);
        this.push_direction = 0;
    }

    attach(player)
    {
        if (!super.attach(player)) return false;
        var i = 0;
        for (; i<player.directions.length && !player.directions[i]; ++i) ;
        this.push_direction = i==player.directions.length ? 0 : (i+2)%player.directions.length;
        if (!player.speeds[this.push_direction] || player.isImune())
        { this.remove_self(player); return true; }
        player.speed = 10;
        this.sound.play();
        return true;
    }

    action(player, level)
    {
        for (let i=0; i<player.directions.length; ++i)
            player.directions[i] = i==this.push_direction;

        if (!player.speeds[this.push_direction])
        {
            player.speed = player.true_speed/(player.isBeingSlowed() ? 2 : 1);
            this.remove_self(player);
        }
    }

    static get_id()
    {
        return 0;
    }

}
