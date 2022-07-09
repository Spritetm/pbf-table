/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


//build_model selects what to render:
//0 - Body, backbox and button assembly cutout
//1 - Body
//2 - Backbox
//3 - Button assembly
//4 - Smattering of buttons. Pick the one that has a comfortable length. Need to cut it at the stem, the flat cylinder at the base is only for printability
//5 - A foot. Need to cut at the stem,the flat cylinder at the base is only for printability. You need four of these.
//Note that the build also uses four 2mm diameter brass rods for the legs, and some 1mm brass rod and a spring in the plunger assembly. The 1mm brass rods are also used to fix the buttons in place.
build_model=0;

mainlcd_w=42.8;
mainlcd_h=74.9;
mainlcd_lip=1;
mainlcd_extra_lip_bottom=5;
mainlcd_th=4.2; //including pcb
toplcd_w=52.8+1;
toplcd_h=46.4;
toplcd_th=2.9; //ex pcb
toplcd_pcbh=48+1.5;
toplcd_conh=9;
toplcd_conw=13.7;
toplcd_pcbth=1.7;
toplcd_conny=mainlcd_h+0.5;
toplcd_lip=1;
toplcd_slotw=(toplcd_pcbth+toplcd_th+0.5);
wall_th=2;
tolerance=0.2;
plunger_fhd=15;
plunger_bhd=15;
plunger_hw=7;
plunger_d=1.2;
plunger_height=26; //height of the actual plunger
plunger_asm_height=13; //height of the assembly
plunger_asm_offset_y=12;

insert_d=5;
insert_h=4;
insert_dist=25;

btn_l=5.9;
btn_w=4.4;
btn_h=1.7; //note: ex smd 'feet'
btn_pdia=1; //plunger dia
btn_plen=2.3; //length of plunger

knob_d=5;
knob_small_d=2.5;

speaker_w=27;
speaker_l=16;
speaker_d=1.5;
speaker_grille_dist=3;
speaker_grille_width=1;
$fn=50;


mscale=(mainlcd_w+(wall_th-mainlcd_lip)*2)/560;
pinball_base_w=560*mscale;
//pinball_base_l=1315*mscale; //official pb measure
pinball_base_l=1020*mscale+1.5; //looks better with lcd
pinball_base_minh=400*mscale;
pinball_base_maxh=605*mscale; //unused; see case_back_top
pinball_playfield_ang=8.875;

backhole_h=25;
backhole_w=pinball_base_w-2*wall_th;
case_back_top=11.0;

topbox_depth=10;

buttons_z=-11;
buttons_y=wall_th*6;

startbutton_len=buttons_y-wall_th+tolerance;

feet_d=2;
feethole_l=10;
feet_ang=12;
fq_th=4; //size of cube that holds the feet hole on the inside of the case

lact_l=15.3;
lact_w=8.2;
lact_h=2.6;
lact_tab_w=4;
lact_tab_off=3.3;


if (build_model==0) {
    intersection() {
        union() {
            pinball_body();
            translate([-mainlcd_w/2,0,-mainlcd_lip]) rotate([  pinball_playfield_ang, 0, 0]) translate([0,wall_th,-mainlcd_th]) scale(0.999) color("red") pcb();
            translate([0,pinball_base_l-topbox_depth,case_back_top]) backbox();

            translate([0,toplcd_conny,case_back_top]) color("red") backbox_stuff();
            translate([0,buttons_y,buttons_z]) button_assembly_thing();
        }
        translate([0,-100,-100]) cube([200,200,200]);
    }
} else if (build_model==1) {
    pinball_body();
} else if (build_model==2) {
    backbox();
} else if (build_model==3) {
    button_assembly_thing();
} else if (build_model==4) {
    translate([0,-6,0]) btnknob(9.5);
    translate([6,-6,0]) btnknob(9.5);
    translate([12,-6,0]) btnknob(8.5);
    translate([0,0,0]) btnknob(8.5);
    translate([6,0,0]) btnknob(7.5);
    translate([12,0,0]) btnknob(7.5);
    translate([0,6,0]) btnknob(8);
    translate([6,6,0]) btnknob(8);
    translate([12,6,0]) btnknob(9);
    translate([0,12,0]) btnknob(9);
    translate([6,12,0]) btnknob(10);
    translate([12,12,0]) btnknob(10);    
} else if (build_model==5) {
    foot();
}


module foot() {
    difference() {
        union() {
            hull() {
                sphere(d=4);
                cylinder(d=3, h=5);
            }
            cylinder(d=3, h=10);
        }
        cylinder(d=2, h=10);
        translate([0,0,7])rotate([90,0,0]) cylinder(d=0.7, h=3);
    }
    translate([0,0,8]) cylinder(d=10, h=2);
}


module pwrknob() {
    difference() {
        union() {
            translate([-1.5,0,-1]) cube([4.5,3.5,6+1]);    
            translate([-1.5-2,-2,-2.7]) cube([4.5+4,3.5+4,2.7]);        translate([-1.5-2,-2.5,-5]) cube([4.5+4,2.5,5]);
        }
        translate([0,0.4,-5]) cube([1.6,2,5]);

    }
}

module btnknob(length) {
    stem_len=8;
    rot_d=10;
    cylinder(h=1, d=knob_d);
    cylinder(h=stem_len+1, d=knob_small_d-tolerance*2);
    translate([0,0,stem_len]) cylinder(h=1, d1=knob_small_d-tolerance*2, d2=knob_d-tolerance*2);
    difference() {
        intersection() {
            translate([0,0,stem_len+1]) cylinder(h=length*2, d=knob_d-tolerance*2);
            translate([0,0,stem_len+length-rot_d/2]) sphere(d=rot_d);
        }
        hull() {
            translate([0,0,stem_len+2.2]) rotate([90,0,0]) cylinder(h=10, d=1.2, center=true);
            translate([0,0,stem_len+2.2+3]) rotate([90,0,0]) cylinder(h=10, d=1.2, center=true);
        }
    }
}


module button_assembly_thing() {
    difference() {
        btnholder_l=btn_l+wall_th/2;
        btnholder_w=knob_d+wall_th/2;
        union() {
           translate([-(btnholder_l+btnholder_w/2), 0, 0]) rotate([0,0,90]) button_assembly_pos(mainlcd_w/2-(btnholder_l+btnholder_w/2)-wall_th/2);
            translate([(btnholder_l+btnholder_w/2), 0, 0]) rotate([0,0,-90]) button_assembly_pos(mainlcd_w/2-(btnholder_l+btnholder_w/2)-wall_th/2);
            rotate([0,0,180]) button_assembly_pos(startbutton_len);
        }
        union() {
            translate([-(btnholder_l+btnholder_w/2), 0, 0]) rotate([0,0,90]) button_assembly_neg(mainlcd_w/2-(btnholder_l+btnholder_w/2)-wall_th/2);
            translate([(btnholder_l+btnholder_w/2), 0, 0]) rotate([0,0,-90]) button_assembly_neg(mainlcd_w/2-(btnholder_l+btnholder_w/2)-wall_th/2);
            rotate([0,0,180]) button_assembly_neg(startbutton_len);
        }
    }
}

module button_assembly_relief_thing() {
    btnholder_l=btn_l+wall_th/2;
    btnholder_w=knob_d+wall_th/2;
    translate([-(btnholder_l+btnholder_w/2), 0, 0]) rotate([0,0,90]) button_assembly_relief(mainlcd_w/2-(btnholder_l+btnholder_w/2)-wall_th/2);
    translate([(btnholder_l+btnholder_w/2), 0, 0]) rotate([0,0,-90]) button_assembly_relief(mainlcd_w/2-(btnholder_l+btnholder_w/2)-wall_th/2);
    rotate([0,0,180]) button_assembly_relief(startbutton_len);

}


module button_assembly_pos(length) {
    //Tray for switch
    translate([-(knob_d+wall_th)/2,-btn_l-wall_th/2,-knob_d+btn_h/2]) cube([knob_d+wall_th, btn_l+wall_th/2+length, knob_d]);
    //thing actual button is in
    translate([-(knob_d+wall_th)/2, wall_th/2, -(knob_d+wall_th)/2]) cube([knob_d+wall_th, length-wall_th/2, knob_d+wall_th]);
}

module button_assembly_neg(length) {
    //cutout for button
    translate([0,-btn_l/2,1]) cube([btn_w, btn_l, btn_h+2], center=true);
    //3mm shaft
    rotate([-90,0,0]) translate([0,0,-.1]) cylinder(d=knob_small_d+tolerance, h=length+1);
    //5mm shaft
    rotate([-90,0,0]) translate([0,0,btn_plen*2]) cylinder(d=knob_d+tolerance, h=length+wall_th*2);
    //square relief
    //translate([-(knob_d+tolerance)/2,btn_plen*2,0]) cube([knob_d+tolerance, length, knob_d]);
    //pin for holding btn
    translate([0,btn_plen*2+2+2.5,0]) rotate([0,90,0]) cylinder(d=1.1, h=knob_d+wall_th*3, center=true);
}

module button_assembly_relief(length) {
    //Tray for switch
    translate([-(knob_d+wall_th)/2-tolerance,-btn_l-wall_th/2,-knob_d+btn_h/2-tolerance]) cube([knob_d+wall_th+tolerance*2, btn_l+wall_th/2+length+tolerance, knob_d+tolerance*2]);
    //thing actual button is in
    translate([-(knob_d+wall_th)/2-tolerance, wall_th/2, -(knob_d+wall_th)/2-tolerance]) cube([knob_d+wall_th+tolerance*2, length-wall_th/2+tolerance, knob_d+wall_th+tolerance*2]);
    //5mm shaft
    rotate([-90,0,0]) translate([0,0,btn_plen*2]) cylinder(d=knob_d+tolerance, h=length+wall_th*2);
}


module screwhole() {
    rotate([-90,0,0]) union() {
        translate([0,0,-10]) cylinder(h=20, d=3.5);
        cylinder(h=10, d=5.3+0.3);
    }
}

module backbox() {
    nonlip=(wall_th-toplcd_lip);
    difference() {
        //outside
        translate([-toplcd_w/2-wall_th,-topbox_depth*3,-wall_th])cube([toplcd_w+wall_th*2, topbox_depth*4, toplcd_pcbh+nonlip+wall_th]);
        //inside
        translate([-toplcd_w/2, -topbox_depth*3-wall_th, 0]) cube([toplcd_w, topbox_depth*4, toplcd_pcbh-toplcd_lip]);
        //cut off so the bezel is angled
        translate([-50, -topbox_depth*4, -wall_th*5]) rotate([5,0,0]) cube([100, topbox_depth*4, toplcd_pcbh*2]);
        //PCB connector cutout
        translate([-mainlcd_w/2-wall_th+mainlcd_lip-tolerance, -wall_th, -wall_th*1.5]) cube([mainlcd_w+(wall_th-mainlcd_lip+tolerance)*2, topbox_depth, wall_th*2]);
        //top LCD cutout.. no idea why 1.6
        translate([-toplcd_w/2,1.6,toplcd_pcbh-toplcd_h]) cube([toplcd_w, toplcd_slotw, toplcd_h]);
        
    }
    difference() {
        //Backplate
        translate([-backhole_w/2, topbox_depth-wall_th, -backhole_h+tolerance]) cube([backhole_w-tolerance, wall_th, backhole_h]);
        //Screwholes
        translate([insert_dist/2, topbox_depth-1, -backhole_h+insert_d/2+wall_th/2]) screwhole();
        translate([-insert_dist/2, topbox_depth-1, -backhole_h+insert_d/2+wall_th/2]) screwhole();
        //pcb for pcb holes
        translate([0,-pinball_base_l+topbox_depth,-case_back_top]) translate([-mainlcd_w/2,0,-mainlcd_lip]) rotate([  pinball_playfield_ang, 0, 0]) translate([0,wall_th,-mainlcd_th]) pcb_cutouts();
    }
    
}


module backbox_stuff() {
    //LCD
    translate([-toplcd_w/2,0,toplcd_pcbh-toplcd_h]) cube([toplcd_w, toplcd_th, toplcd_h]);
    //PCB + conn end
    translate([-toplcd_w/2,toplcd_th,0]) cube([toplcd_w, toplcd_pcbth, toplcd_pcbh]);
    translate([-toplcd_conw/2,toplcd_th,-toplcd_conh]) cube([toplcd_conw, toplcd_pcbth, toplcd_pcbh]);
}


module pcb() {
    union() {
        cube([mainlcd_w, mainlcd_h, mainlcd_th]);
        translate([mainlcd_lip,0,-2]) cube([mainlcd_w-mainlcd_lip*2, mainlcd_h, mainlcd_th]);
        translate([mainlcd_lip,24,-10]) cube([mainlcd_w-mainlcd_lip*2, mainlcd_h-24, mainlcd_th+9]);
    }
}

module pcb_cutouts() {
    union() {
        cube([mainlcd_w, mainlcd_h, mainlcd_th]);
        translate([mainlcd_lip,0,-2]) cube([mainlcd_w-mainlcd_lip*2, mainlcd_h, mainlcd_th]);
        translate([mainlcd_lip,24,-10]) cube([mainlcd_w-mainlcd_lip*2, mainlcd_h-24, mainlcd_th+9]);
        //usb
        translate([mainlcd_w-8, mainlcd_h, mainlcd_th-5]) cube([10, 30, 7], center=true);
        //switch
        translate([7, mainlcd_h, mainlcd_th-5]) cube([8, 30,4], center=true);
    }
}


module insert_bar() {
    translate([0,-insert_h/2,0]) difference() {
    cube([pinball_base_w, insert_h, insert_d+wall_th*2], center=true);
        translate([-insert_dist/2,0,0])rotate([90,0,0]) cylinder(d=insert_d, h=insert_h+1, center=true);
        translate([insert_dist/2,0,0])rotate([90,0,0]) cylinder(d=insert_d, h=insert_h+1, center=true);
    }
}


module plunger_asm(height) {
    rotate([-90,0,90]) difference() {
        cube([plunger_fhd+plunger_bhd+wall_th*3, height, wall_th*2+plunger_hw]);
        translate([wall_th, -1, wall_th]) cube([plunger_fhd, height+2, plunger_hw]);
        translate([wall_th+plunger_fhd+wall_th, -1, wall_th]) cube([plunger_bhd, height+2, plunger_hw]);
    }
}

module plunger_rod_hole() {
    translate([mainlcd_w/2-mainlcd_lip-plunger_hw/2, 0, -pinball_base_minh]) rotate([-90,0,0]) cylinder(h=plunger_fhd+wall_th*2+2, d=plunger_d);
}

//Cutout for the speaker and its grille
module speaker_ex() {
    translate([speaker_w/2,0,0]) rotate([0,0,90]) difference() {
        translate([0,0,-wall_th*2])cube([speaker_l, speaker_w, wall_th*4]);
        union() {
            translate([-speaker_w*0.7, speaker_l*0.7, -(speaker_grille_width+speaker_d+wall_th)]) for (x=[0:speaker_grille_dist:speaker_w]) {
                translate([x,x,0]) rotate([0,0,-45]) cube([speaker_w*1.4, speaker_grille_width, speaker_grille_width+speaker_d+wall_th]);
                translate([x,-x,0]) rotate([0,0,45]) cube([speaker_w*1.4, speaker_grille_width, speaker_grille_width+speaker_d+wall_th]);
            }
        }
    }
}

module btn() {
    translate([0,-btn_l/2,0]) cube([btn_w, btn_l, btn_h], center=true);
    rotate([90,0,0]) cylinder(d=btn_pdia, h=btn_plen*2, center=true);
}


module feethole(rot) {
    rotate([feet_ang, 0, 45+90*rot]) cylinder(d=feet_d+tolerance, h=feethole_l);
}


module lact_holder() {
    difference() {
        translate([-wall_th/2, -wall_th/2, -wall_th]) cube([lact_l+wall_th, lact_w+wall_th, lact_h+wall_th]);
        cube([lact_l, lact_w, lact_h+1]);
        translate([lact_tab_off, wall_th, 0]) cube([lact_tab_w, lact_w, lact_h+1]);
    }
}


module rib(w, h, angle1=10, angle2=20) {
    intersection() {
        cube([w,wall_th,h]);
        translate([wall_th, 0,wall_th]) union() {
            translate([0,0,h]) rotate([0,-angle1,0]) translate([-w,0,-h]) cube([w,wall_th,h]);
            translate([w,0,0]) rotate([0,angle2,0]) translate([-w,0,-h]) cube([w,wall_th,h]);
        }
    }
}


module pinball_body() {
    difference() {
        pinball_body_part();
        //feet holes
        translate([(pinball_base_w/2)-2, 2, -pinball_base_minh-1]) feethole(2);
        translate([-(pinball_base_w/2)+2, 2, -pinball_base_minh-1]) feethole(1);
        translate([-(pinball_base_w/2)+2, pinball_base_l-2, -pinball_base_minh-1]) feethole(0);
        translate([(pinball_base_w/2)-2, pinball_base_l-2, -pinball_base_minh-1]) feethole(3);
    }
    translate([-mainlcd_w/2-wall_th/2, 55, -pinball_base_minh]) rib(mainlcd_w/3, pinball_base_minh, angle2=30);
    translate([mainlcd_w/2+wall_th/2, 55, -pinball_base_minh]) rotate([0,0,180])rib(mainlcd_w/3, pinball_base_minh, angle2=30);
    translate([-mainlcd_w/2-wall_th/2, 35, -pinball_base_minh]) rib(mainlcd_w/3, (pinball_base_minh-4), angle2=30);
}


module pinball_body_part() {
    difference() {
        //body cube
        translate([-pinball_base_w/2, 0, -pinball_base_minh]) cube([pinball_base_w, pinball_base_l, pinball_base_maxh]);
        //angled cut for playfield
        rotate([pinball_playfield_ang, 0, 0]) translate([-50,0,0]) cube([100,150,100]);
        //hollow out, minus feet cubes
        difference() {
            translate([-pinball_base_w/2+wall_th, wall_th, -pinball_base_minh+wall_th]) cube([pinball_base_w-wall_th*2, pinball_base_l-wall_th*2, pinball_base_maxh]);
            translate([(pinball_base_w/2)-wall_th-fq_th, wall_th, -pinball_base_minh+wall_th]) cube([fq_th, fq_th, feethole_l]);
            translate([-(pinball_base_w/2)+wall_th, wall_th, -pinball_base_minh+wall_th]) cube([fq_th, fq_th, feethole_l]);
            translate([-(pinball_base_w/2)+wall_th, pinball_base_l-wall_th-fq_th, -pinball_base_minh+wall_th]) cube([fq_th, fq_th, feethole_l]);
            translate([(pinball_base_w/2)-wall_th-fq_th, pinball_base_l-wall_th-fq_th, -pinball_base_minh+wall_th]) cube([fq_th, fq_th, feethole_l]);
        }
        //cut out lcd plus pcb space
         translate([-mainlcd_w/2,0,-mainlcd_lip]) rotate([pinball_playfield_ang, 0, 0]) translate([0,wall_th,-mainlcd_th]) cube([mainlcd_w, pinball_base_l+2, mainlcd_th]);        
        //flatten top
        translate([-25, 0, case_back_top]) cube([50,100, 10]);
        //cut out bits for back box back
        translate([-25, pinball_base_l-(wall_th+tolerance), case_back_top-(wall_th+tolerance)]) cube([50,10, 10]);
        //cut out back hole
        translate([-backhole_w/2, pinball_base_l-5, case_back_top-backhole_h]) cube([backhole_w, 10, backhole_h+1]);
        //plunger hole
        translate([0,-1,plunger_height]) plunger_rod_hole();
        //temp plunger hole, for insertion of rod into plunger assembly
        translate([0,-1,plunger_asm_height]) plunger_rod_hole();
        //cut out speaker hole and grille
        translate([0,pinball_base_l-speaker_l-wall_th-insert_h,-pinball_base_minh+wall_th-speaker_d]) speaker_ex();
        //button assembly holes
        translate([0,buttons_y,buttons_z]) button_assembly_relief_thing();
        //airholes
        translate([0,-1,-pinball_base_minh+wall_th+1]) rotate([-90,0,0])cylinder(h=100,d=1.5);
    }
    //plunger assembly
    difference() {
        translate([mainlcd_w/2+mainlcd_lip, wall_th+plunger_asm_offset_y, -pinball_base_minh+plunger_asm_height+plunger_hw/2]) plunger_asm(plunger_asm_height+plunger_hw/2);
        translate([0,plunger_asm_offset_y,plunger_asm_height])plunger_rod_hole();
    }
    //bar for screw inserts
    translate([0,pinball_base_l-wall_th, case_back_top-backhole_h+insert_d/2+wall_th/2]) insert_bar();
    //bar over bottom of playfield glass
    translate([-mainlcd_w/2,0,0]) rotate([pinball_playfield_ang, 0, 0]) translate([0,wall_th,-wall_th/2]) cube([mainlcd_w, mainlcd_extra_lip_bottom, wall_th/2]);        
    //linear actuators
    translate([-11,5,-pinball_base_minh+wall_th]) lact_holder();
    translate([6,15,-pinball_base_minh+wall_th]) rotate([0,0,90])lact_holder();
}

