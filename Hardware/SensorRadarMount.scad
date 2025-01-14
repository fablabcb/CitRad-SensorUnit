mainH = 12; // main body height
mainD = 45.9; // main body depth
mainW = 17.78 * 2; // main body width
roundingD = 6; // depth of the front/back rounding thingy
roundingHoleD = 2.8; // diameter of the back/front holes

sensorD = 44.3; // outer depth of the sensor plate
sensorW = 30.3; // outer width of the sensor plate
sensorFullSizeH = 3; // depth of the cutout for the sensor plate
sensorLipW = 1; // width of the lip for the sensor to rest on
sensorFullH = sensorFullSizeH + 6; // depth of the entire sensor cutout

smallSlotW = 12; // width of the small slot
smallSlotX = 10.5; // x offset (from outer edge)
bigSlotW = 17; // width of the big slot
bigSlotX = 8; // x offset (from outer edge)
sensorSlotD = 4; // depth of the header slots for the sensors

$fn=50;
e=0.01;

// xz-origin: center of the hole
module SideThingy()
{
    difference()
    {
        union()
        {
            cube([mainW/2,roundingD,mainH]);
            translate([0,0,mainH/2]) rotate([-90,0,0]) cylinder(d=mainH, h=roundingD);
        }
        
        translate([0,0,mainH/2]) rotate([-90,0,0]) cylinder(d=roundingHoleD, h=roundingD+e);
    }
}

module CompleteHolder()
{
    translate([0,-mainD/2,0]) difference() 
    {
        // main body
        cube([mainW, mainD, mainH]);
        // flat sensor part cutoff
        translate([1.36,(mainD-sensorD)/2,0]) cube([sensorW, sensorD, sensorFullSizeH]);
        // round sensor part cutoff
        translate([1.36+sensorLipW,(mainD-sensorD)/2+sensorLipW,0])
            cube([sensorW-2*sensorLipW, sensorD-2*sensorLipW, sensorFullH]);
        
        // small header slot
        translate([smallSlotX,sensorLipW+sensorD-sensorSlotD,-1]) cube([smallSlotW,sensorSlotD,99*mainH]);
        // big slot and push-through area
        translate([bigSlotX,sensorLipW,-1]) cube([bigSlotW,sensorD-10-sensorSlotD,99*mainH]);
        // holes
        hR=1.4;
        holes=[[1.9+hR,5.8+hR],[mainW-1.9-hR,5.8+hR],[14.6+hR,mainD-7.07-hR]];
        for(h=holes)
            translate([h[0],h[1],mainH-3]) cylinder(r=1.4, h=3+e);
        // groves for teensy pins
        for(y=[12.8,12.8+15.24])
            translate([0,y,mainH]) rotate([0,90,0]) cylinder(r=2, h=mainW, $fn=7);
        
        translate([mainW*0.73,mainD/2-5,mainH-1]) linear_extrude(1) text("v2",size=6);
    }
    
    translate([mainW/2,0,0])
    {
        translate([0,mainD/2,0]) SideThingy();
        mirror([0,1,0])
            translate([0,mainD/2,0]) SideThingy();
    }
}

CompleteHolder();