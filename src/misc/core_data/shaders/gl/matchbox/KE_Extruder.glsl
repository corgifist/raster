
/*

    Extruded Video Image
    --------------------

	I made this after looking at another one of Flockaroo's cool video effects,
	namely his "Voxel Video" example. It's based on basic extruded grid code
	that I've used from time to time. I wanted to see how things would look 
	taking a raymarched extruded approach. It lacks some of the multilevel
    subdivided freakiness that you get with Flockaroo's example, but it's still 
	reasonably interesting.

	I originally started with one square size, but it was difficult to descern
	the image for all but the smallest of grid cell dimensions, so I used the 
	usual corner pixel techniques to subdivide down one level. The downside, of 
    course, was forcing the GPU to perform more work. That could be mitigated
	by precalculating on a buffer, etc, but I wanted to keep the example 
    relatively simple by restricting things to a single pass. Some might also
	note that I've subdivided one object directly into four small squares, and 
	left out the rectangles. I did that for simplicity sake, but a "power of
    2" subdivision would normally include those.

	Anyway, I did this for a bit of fun, and to satisfy my curiosity. I could 
	make a lot of improvements, but this will do for now. Later, I might start
	with larger block sizes, then divide down a couple more levels, just to see 
	if it looks more like Flockaroo's original. However, dealing with 
	neighboring subdivided grid cells in a raymarching environment can be 
	tricky, not to mention expensive, so I might try that on the 2D plane first.
	
    

	Inspired by:
    
	// As mentioned, Flockaroo makes all kinds of interesting video processing
	// examples, and this is just one of them.
    voxel video - flockaroo
	https://www.shadertoy.com/view/3stXzB  


*/

// Ported to Flame Matchbox by Ted Stanley (KuleshovEffect) - October, 2022
// v1.02

// Increased the visible resolution, where applicable. Uncomment to see what
// a big diference it makes.
#define SUBDIVIDE

// Sparkles, or no sparkles.
//#define SPARKLES

// Grayscale, for that artsy look.
//#define GRAYSCALE



uniform sampler2D back;
uniform float adsk_back_w, adsk_back_h;
uniform float adsk_result_w, adsk_result_h;
uniform float adsk_time;
uniform float farness;
uniform bool sparks, gray;
//float iTime = adsk_time / 24.0;
//int iFrame = int(adsk_time);

//vec2 iResolution = vec2(adsk_result_w, adsk_result_h);

// Scene object ID to separate the mesh object from the terrain.
float objID = 0.;

// Max ray distance.
#define FAR farness
// #define FAR 20

// Standard 2D rotation formula.
mat2 rot2(in float a){ float c = cos(a), s = sin(a); return mat2(c, -s, s, c); }


// IQ's vec2 to float hash.
float hash21(vec2 p){  return fract(sin(dot(p, vec2(27.609, 57.583)))*43758.5453); }


// Getting the video texture. I've deliberately stretched it out to fit across the screen,
// which means messing with the natural aspect ratio.
//
// By the way, it'd be nice to have a couple of naturally wider ratio videos to choose from. :)
//
vec3 getTex(vec2 p){
    
    // Strething things out so that the image fills up the window. You don't need to,
    // but this looks better. I think the original video is in the oldschool 4 to 3
    // format, whereas the canvas is along the order of 16 to 9, which we're used to.
    // If using repeat textures, you'd comment the first line out.
    vec2 iResolution = vec2(adsk_result_w, adsk_result_h);
    p *= vec2(iResolution.y/iResolution.x, 1);
    vec3 tx = texture2D(back, fract(p/2. - .5)).xyz;
    return tx*tx; // Rough sRGB to linear conversion.
}

// Height map value, which is just the pixel's greyscale value.
float hm(in vec2 p){ return dot(getTex(p), vec3(.299, .587, .114)); }

// IQ's extrusion formula.
float opExtrusion(in float sdf, in float pz, in float h){
    
    vec2 w = vec2( sdf, abs(pz) - h );
  	return min(max(w.x, w.y), 0.) + length(max(w, 0.));

    /*
    // Slight rounding. A little nicer, but slower.
    const float sf = .025;
    vec2 w = vec2( sdf, abs(pz) - h) + sf;
  	return min(max(w.x, w.y), 0.) + length(max(w, 0.)) - sf;
    */
}


// IQ's unsigned box formula.
float sBoxS(in vec2 p, in vec2 b, in float sf){

  return length(max(abs(p) - b + sf, 0.)) - sf;
}

/*
// IQ's signed box formula.
float sBoxS(in vec2 p, in vec2 b, in float sf){
   

  vec2 d = abs(p) - b + sf;
  return min(max(d.x, d.y), 0.) + length(max(d, 0.)) - sf;
}
*/
 
// A regular extruded block grid, subdivided down one level, if necessary.
//
// The idea is very simple: Produce a normal grid full of packed square pylons.
// That is, use the grid cell's center pixel to obtain a height value (read in
// from a height map), then render a pylon at that height.
//
// For the subdivision step, split each square into four smaller squares, obtain
// the heights, then check each against the main height to see whether any
// exceed a certain threshold. If any do, render the four smaller pylons at
// their respective heights. In theory, you could keep going this way, but not
// on a single pass, because you'd soon fry your GPU. :)
// 
vec4 blocks(vec3 q3){
    
    // Scale.
    const float scale = 1./16.;

    // Brick dimension: Length to height ratio with additional scaling.
	const vec2 l = vec2(scale);
    // A helper vector, but basically, it's the size of the repeat cell.
	const vec2 s = l*2.;
    
    // Distance.
    float d = 1e5;
    // Cell center, local coordinates and overall cell ID.
    vec2 p, ip;
    
    // Individual brick ID.
    vec2 id = vec2(0);
    vec2 cntr = vec2(0);
    
    // Four block corner postions.
    vec2[4] ps4 = vec2[4](vec2(-l.x, l.y), l, -l, vec2(l.x, -l.y));
    
    float boxID = 0.; // Box ID. Not used in this example, but helpful.
    
    for(int i = 0; i<4; i++){

        // Block center.
        cntr = ps4[i]/2.;


        // Local coordinates.
        p = q3.xy - cntr;
        ip = floor(p/s) + .5; // Local tile ID.
        p -= (ip)*s; // New local position.

       
        // Correct positional individual tile ID.
        vec2 idi = ip*s + cntr;
 
        // The extruded block height. See the height map function, above.
        float h = hm(idi);
        #ifndef SUBDIVIDE
        h = floor(h*15.999)/15.*.15; // Or just, "h *= .15," for nondiscreet heights.
        #endif
        
        #ifdef SUBDIVIDE
        // Subdivide the block into four smaller blocks, then check it's height
        // against the main block height (above). If any of the height differences
        // exceed a threshold (I've chosen one block height unit), then flag then
        // main block for subdivision.
        vec4 h4;
        int sub = 0;
        for(int j = 0; j<4; j++){
            h4[j] = hm(idi + ps4[j]/4.);
            if(abs(h4[j] - h)>1./15.) sub = 1;
        }
        
        
        // Using the floor function to snap the heights to specific quantized values.
        h = floor(h*15.999)/15.*.15;
        h4 = floor(h4*15.999)/15.*.15;
        
        // Without discreet heights -- Comment out the two lines above first though.
        //h *= .15;
        //h4 *= .15;
         
        
        // If subdividing, render the four smaller blocks at their respective
        // heights. Otherwise, render the one larger block. Refer to the image
        // for a visual representation.
        if(sub==1){
            
            // Four smaller extruded blocks.
            vec4 d4, di4;

            for(int j = 0; j<4; j++){
                d4[j] = sBoxS(p - ps4[j]/4., l/4. - .05*scale, .005);
                di4[j] = opExtrusion(d4[j], (q3.z + h4[j]), h4[j]);
                
                // If applicable, update the overall minimum distance value,
                // ID, and box ID.
                if(di4[j]<d){
                    d = di4[j];
                    id = idi + ps4[j]/4.;
                    // Not used in this example, so we're saving the calulation.
                    //boxID = float(j)*4. + float(i);
        		}
            }
        }
        else {
        #endif
            
            // One larger extruded block.
            float di2D = sBoxS(p, l/2. - .05*scale, .015);
            
            // Boring out some of the lower boxes. I like it, but thought it
            // confused matters.
            //if(h<1./15.*.15 + .001) di2D = max(di2D, -(di2D + .3*scale));
            
        	// The extruded distance function value.
            float di = opExtrusion(di2D, (q3.z + h), h);
            
            // If applicable, update the overall minimum distance value,
                // ID, and box ID. 
            if(di<d){
                d = di;
                id = idi;
                // Not used in this example, so we're saving the calulation.
                //boxID = float(i);
        	}
            
        #ifdef SUBDIVIDE    
        }
        #endif
        
    }
    
    // Return the distance, position-base ID and box ID.
    return vec4(d, id, boxID);
}


// Block ID -- It's a bit lazy putting it here, but it works. :)
vec2 gID = vec2(0.);

// The extruded image.
float map(vec3 p){
    
    // Floor.
    float fl = -p.z + .1;

    // The extruded blocks.
    vec4 d4 = blocks(p);
    gID = d4.yz; // Individual block ID.
    
 
    // Overall object ID.
    objID = fl<d4.x? 1. : 0.;
    
    // Combining the floor with the extruded image
    return  min(fl, d4.x);
 
}

 
// Basic raymarcher.
float trace(in vec3 ro, in vec3 rd){

    // Overall ray distance and scene distance.
    float t = 0., d;
    int iFrame = int(adsk_time);
    
    for(int i = int(min(float(iFrame), 0.0)); i<64; i++){
    
        d = map(ro + rd*t);
        // Note the "t*b + a" addition. Basically, we're putting less emphasis on accuracy, as
        // "t" increases. It's a cheap trick that works in most situations... Not all, though.
        if(abs(d)<.001 || t>float(FAR)) break; // Alternative: 0.001*max(t*.25, 1.), etc.
        
        //t += i<32? d*.75 : d; 
        t += d*.7; 
    }

    return min(t, float(FAR));
}


// Standard normal function. It's not as fast as the tetrahedral calculation, but more symmetrical.
vec3 getNormal(in vec3 p, float t) {
	const vec2 e = vec2(.001, 0);
	return normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy),	map(p + e.yyx) - map(p - e.yyx)));
}


// Cheap shadows are hard. In fact, I'd almost say, shadowing particular scenes with limited 
// iterations is impossible... However, I'd be very grateful if someone could prove me wrong. :)
float softShadow(vec3 ro, vec3 lp, vec3 n, float k){

    // More would be nicer. More is always nicer, but not really affordable... Not on my slow test machine, anyway.
    const int maxIterationsShad = 24; 
    
    ro += n*.0015;
    vec3 rd = lp - ro; // Unnormalized direction ray.
    

    float shade = 1.;
    float t = 0.;//.0015; // Coincides with the hit condition in the "trace" function.  
    float end = max(length(rd), 0.0001);
    //float stepDist = end/float(maxIterationsShad);
    rd /= end;

    int iFrame = int(adsk_time);

    // Max shadow iterations - More iterations make nicer shadows, but slow things down. Obviously, the lowest 
    // number to give a decent shadow is the best one to choose. 
    for (int i = int(min(float(iFrame), 0.)); i<maxIterationsShad; i++){

        float d = map(ro + rd*t);
        shade = min(shade, k*d/t);
        //shade = min(shade, smoothstep(0., 1., k*h/dist)); // Subtle difference. Thanks to IQ for this tidbit.
        // So many options here, and none are perfect: dist += min(h, .2), dist += clamp(h, .01, stepDist), etc.
        t += clamp(d, .01, .25); 
        
        
        // Early exits from accumulative distance function calls tend to be a good thing.
        if (d<0. || t>end) break; 
    }

    // Sometimes, I'll add a constant to the final shade value, which lightens the shadow a bit --
    // It's a preference thing. Really dark shadows look too brutal to me. Sometimes, I'll add 
    // AO also just for kicks. :)
    return max(shade, 0.); 
}


// I keep a collection of occlusion routines... OK, that sounded really nerdy. :)
// Anyway, I like this one. I'm assuming it's based on IQ's original.
float calcAO(in vec3 p, in vec3 n)
{
	float sca = 3., occ = 0.;
    for( int i = 0; i<5; i++ ){
    
        float hr = float(i + 1)*.15/5.;        
        float d = map(p + n*hr);
        occ += (hr - d)*sca;
        sca *= .7;
    }
    
    return clamp(1. - occ, 0., 1.);  
    
    
}


void main( void ){

    
    // Screen coordinates.
    vec2 iResolution = vec2(adsk_result_w, adsk_result_h);
	vec2 uv = (gl_FragCoord.xy - iResolution.xy*.5)/iResolution.y;
	
	float iTime = adsk_time / 24.0;
    
    // Camera Setup.
	vec3 lk = vec3(0, 0, 0);//vec3(0, -.25, iTime);  // "Look At" position.
	// vec3 ro = lk + vec3(-.5*.3*cos(iTime/2.), -.5*.2*sin(iTime/2.), -2); // Camera position, doubling as the ray origin.
    vec3 ro = lk + vec3(vec2(0.), -2.);

    // Light positioning. One is just in front of the camera, and the other is in front of that.
 	vec3 lp = ro + vec3(1.5, 2, -1);// Put it a bit in front of the camera.
	

    // Using the above to produce the unit ray-direction vector.
    float FOV = 1.; // FOV - Field of view.
    vec3 fwd = normalize(lk-ro);
    vec3 rgt = normalize(vec3(fwd.z, 0., -fwd.x )); 
    // "right" and "forward" are perpendicular, due to the dot product being zero. Therefore, I'm 
    // assuming no normalization is necessary? The only reason I ask is that lots of people do 
    // normalize, so perhaps I'm overlooking something?
    vec3 up = cross(fwd, rgt); 

    // rd - Ray direction.
    vec3 rd = normalize(fwd + FOV*uv.x*rgt + FOV*uv.y*up);
    
    // Swiveling the camera about the XY-plane.
	//rd.xy *= rot2( sin(iTime)/32. );

    
/*      
    // Mouse controls.   
	vec2 ms = vec2(0);
    if (iMouse.z > 1.0) ms = (iMouse.xy - iResolution.xy*.5)/iResolution.xy;
    vec2 a = sin(vec2(1.5707963, 0) - ms.x); 
    mat2 rM = mat2(a, -a.y, a.x);
    rd.xz = rd.xz*rM; 
    a = sin(vec2(1.5707963, 0) - ms.y); 
    rM = mat2(a, -a.y, a.x);
    rd.yz = rd.yz*rM;
*/    
	 
    
    // Raymarch to the scene.
    float t = trace(ro, rd);
    
    // Save the block ID and object ID.
    vec2 svGID = gID;
    
    float svObjID = objID;
  
	
    // Initiate the scene color to black.
	vec3 col = vec3(0);
	
	// The ray has effectively hit the surface, so light it up.
	if(t < float(FAR)){
        
  	
    	// Surface position and surface normal.
	    vec3 sp = ro + rd*t;
	    //vec3 sn = getNormal(sp, edge, crv, ef, t);
        vec3 sn = getNormal(sp, t);
        
          
        // Obtaining the texel color. 
	    vec3 texCol;   

        // The extruded grid.
        if(svObjID<.5){
            
            // Coloring the individual blocks with the saved ID.
            vec3 tx = getTex(svGID);
            //vec3 tx = getTex(sp.xy - .5/16.); // See scale in the distance function.
            // Greyscale value, just in case people switch to the Britney video, etc.
            // Stylistically, the example works better with color. The Britney video
            // looks OK, but I'm more of a Shirley Jones kind of guy. :)
            //#ifdef GRAYSCALE
            if ( gray )
            {
                texCol = vec3(1)*dot(tx, vec3(.299, .587, .114));
            }
            else{
                texCol = tx;
            }
            //#endif
            
            
            if ( sparks )
            {
            
                // Putting some blinking colored dots in the background. I did this to liven
                // things up a bit. It's a little quirky, but looks... interesting, I guess. :D
                float rnd = fract(sin(dot((svGID), vec2(141.13, 289.97)))*43758.5453);
                float rnd2 = fract(sin(dot((svGID + .037), vec2(141.13, 289.97)))*43758.5453);
                rnd = smoothstep(.9, .95, cos(rnd*6.283 + iTime*2.)*.5 + .5);
                vec3 rndCol = (.5 + .45*cos(6.2831*mix(0., .3, rnd2) + vec3(0, 1, 2)/1.1));
                rndCol = mix(rndCol, rndCol.xzy, uv.y*.75 + .5);
                rndCol = mix(vec3(1), rndCol*50., rnd*smoothstep(1. - (1./1./15. + .001), 1., 1. - texCol.x));
                
                texCol *= rndCol;
                
                //#endif
            }
            
            // Ramping the shade up a bit.
            texCol = smoothstep(0., 1., texCol);
 
        }
        else {
            
            // The dark floor in the background. Hiddent behind the pylons, but
            // you still need it.
            texCol = vec3(0);
        }
       
    	
    	// Light direction vector.
	    vec3 ld = lp - sp;

        // Distance from respective light to the surface point.
	    float lDist = max(length(ld), .001);
    	
    	// Normalize the light direction vector.
	    ld /= lDist;

        
        
        // Shadows and ambient self shadowing.
    	float sh = softShadow(sp, lp, sn, 8.);
    	float ao = calcAO(sp, sn); // Ambient occlusion.
        sh = min(sh + ao*.25, 1.);
	    
	    // Light attenuation, based on the distances above.
	    float atten = 1./(1. + lDist*.05);

    	
    	// Diffuse lighting.
	    float diff = max( dot(sn, ld), 0.);
        //diff = pow(diff, 4.)*2.; // Ramping up the diffuse.
    	
    	// Specular lighting.
	    float spec = pow(max(dot(reflect(ld, sn), rd ), 0.), 16.); 
	    
	    // Fresnel term. Good for giving a surface a bit of a reflective glow.
        float fre = pow(clamp(dot(sn, rd) + 1., 0., 1.), 2.);
        
        
        // Combining the above terms to procude the final color.
        col = texCol*(diff + ao*.3 + vec3(.25, .5, 1)*diff*fre*16. + vec3(1, .5, .2)*spec*2.);

        // Shading.
        col *= ao*sh*atten;
        
        
	
	}
    
    //float rnd = hash21(rd.xy + fract(iTime));
    //col = clamp(col + (rnd*rnd - .5)*.1, 0., 1.);
          
    
    // Rought gamma correction.
	gl_FragColor = vec4(sqrt(max(col, 0.)), 1);
    // gl_FragColor = texture(back, gl_FragCoord.xy / vec2(adsk_result_w, adsk_result_h));
}