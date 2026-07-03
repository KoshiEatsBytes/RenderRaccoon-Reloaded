
#pragma once
#include <cstdint>

#include "BiomeID.h"

namespace WORLDGEN
{
    // Per-biome vegetation densities
    struct BiomeVeg
    {
        float tree          = 0.0f;
        float grass         = 0.0f;
        float tallGrass     = 0.0f;  // fraction of short grass that becomes tall grass
        float flower        = 0.0f;
        float bush          = 0.0f;
        float cactus        = 0.0f;
        float boulder       = 0.0f;
        float clumpAmount   = 0.0f;  // 0 = uniform scatter, 1 = full clump field
        int   treeMinHeight = 0;     // trunk height range in blocss
        int   treeMaxHeight = 0;
    };

    // Generation configuration panel
    struct WorldGenConfig
    {
        using uInt32 = std::uint32_t;

        // GENERAL -----------------------------------------------------------------------------------------------------
        uInt32 seed = 3053828723;

        // BASE NOISE --------------------------------------------------------------------------------------------------
        bool  useGradientNoise = true;  // gradient (Perlin) vs value noise
        float heightScale      = 128.f; // base-noise wavelength in blocks
        int   heightOctaves    = 5;
        int   dirtDepth        = 4;     // soil thickness under the surface

        // TERRAIN SHAPE -----------------------------------------------------------------------------------------------
        // Domain warp - sinuous landforms
        bool  warpEnabled = true;
        float warpScale   = 400.0f;
        float warpAmp     = 100.f;
        int   warpOctaves = 3;
        int   warpLevels  = 2;          // 1 = single warp, 2 = recursive 
        // Detail / deterracing roughness on the edges
        bool  detailEnabled = true;
        float detailScale   = 48.0f;
        float detailAmp     = 2.0f;     // blocks of edge jitter
        int   detailOctaves = 3;
        // Mountain ridges 
        bool  ridgeMountains = true;
        float ridgeStrength  = 1.f;     // 0 = round, 1 = sharp

        // CLIMATE ----------------------------------------------------------------------------------------------------
        float tempCold          = 0.40f;  // < this = cold band 
        float tempHot           = 0.60f;  // > this = hot band  
        float mountainChance    = 0.2f;   // fraction of coarse cells rolled as mountains
        float tundraHumidThresh = 0.5f;   // cold: < tundra else taiga
        float plainsHumidThresh = 0.5f;   // temperate: < plains else forest
        float desertHumidThresh = 0.5f;   // hot: < desert else savanna
        float mesaRarity        = 0.75f;  // 1 = rarest

        // BIOME MAP --------------------------------------------------------------------------------------------------
        int biomeZoomLevels   = 8;  // coarse-cell size 
        int biomeSmoothPasses = 2;  // majority-vote passes
        int biomeFuzzyLevels  = 1;  // coarsest levels 

        // PER-BIOME TERRAIN ------------------------------------------------------------------------------------------
        int biomeBaseHeight[static_cast<int>(BIOME::COUNT)] = {
            66,  // PLAINS
            71,  // FOREST
            64,  // DESERT
            125,  // MESA
            65,  // TAIGA
            65,  // TUNDRA
            102, // MOUNTAINS
            63,  // SAVANNA
        };
        int biomeAmplitude[static_cast<int>(BIOME::COUNT)] = {
            10,  // PLAINS
            16,  // FOREST
            7,   // DESERT
            10,  // MESA
            35,  // TAIGA
            12,  // TUNDRA
            105, // MOUNTAINS
            10,  // SAVANNA
        };

        // VEGETATION --------------------------------------------------------------------------------------------------

        // Per biome vegetation density
        BiomeVeg biomeVegetation[static_cast<int>(BIOME::COUNT)] = {
            /* PLAINS    */ { 0.0008f, 0.28f,  0.10f, 0.020f, 0.000f, 0.000f, 0.000f, 0.30f,  5,  8 },
            /* FOREST    */ { 0.050f,  0.03f,  0.00f, 0.005f, 0.020f, 0.000f, 0.000f, 0.00f,  5, 10 },
            /* DESERT    */ { 0.000f,  0.03f,  0.00f, 0.000f, 0.014f, 0.004f, 0.000f, 0.00f,  0,  0 },
            /* MESA      */ { 0.000f,  0.015f, 0.00f, 0.000f, 0.020f, 0.000f, 0.000f, 0.00f,  0,  0 },
            /* TAIGA     */ { 0.0062f, 0.03f,  0.00f, 0.000f, 0.005f, 0.000f, 0.006f, 0.27f, 24, 30 },
            /* TUNDRA    */ { 0.0024f, 0.028f, 0.00f, 0.003f, 0.000f, 0.000f, 0.000f, 0.40f,  6,  8 },
            /* MOUNTAINS */ { 0.000f,  0.07f,  0.00f, 0.000f, 0.000f, 0.000f, 0.000f, 0.00f,  0,  0 },
            /* SAVANNA   */ { 0.006f,  0.084f, 0.00f, 0.007f, 0.000f, 0.000f, 0.000f, 0.15f,  2,  4 },
        };

        // Trees
        int treeSlopeMax = 3;

        // Tree clumping biome density
        bool  clumpEnabled = true;
        float clumpScale   = 90.0f;   // grove/clearing size in blocks
        float clumpWarp    = 35.0f;   // edge organicness, lower the rounder
        float clumpClear   = 0.42f;   // field below this clear trees
        float clumpThick   = 0.60f;   // field above this full density trees
        float clumpPeak    = 2.0f;    // thicket multiplier, stick around 2

        // TERRAIN BLEND -----------------------------------------------------------------------------------------------
        int   biomeBlendRadius = 20;     // neighbourhood radius for base/amp blending
        float mountainCurve    = 2.5f;   // foothill steepness
        // Aprons, skirts before rise/cliff
        bool  mesaApron       = true;
        float mesaApronThresh = 0.70f;
        bool  mtnApron        = true;
        float mtnApronThresh  = 0.2f;

        // MOUNTAIN SURFACE --------------------------------------------------------------------------------------------
        int   snowLine        = 150;     // Y above which surface is snow
        int   mtnGrassLine    = 112;     // Y below which surface is grass
        float snowJitterScale = 40.0f;   // wavelength of the wavy snow/grass line
        float snowJitterAmp   = 12.0f;   // blocks the line wobbles (hehe wobble fun word)

        // MESA SURFACE ------------------------------------------------------------------------------------------------
        bool  mesaRimCliffs       = true;
        float mesaNoiseScale      = 260.0f;  // broad makes gentle slopes, narrow wide plateaus
        int   mesaNoiseOctaves    = 3;       // fewer is smoother
        bool  mesaMtnBuffer       = true;    // drop plains where mesa meets mountains
        int   mesaMtnBufferRadius = 1;
        //Mesa Cliffs
        bool  cliffsEnabled       = true;
        float cliffScale          = 256.0f; // cliff cluster size within a mesa
        int   cliffOctaves        = 2;
        float cliffThreshold      = 0.15f;  // cliffNoise above this is cliffy
        float cliffBlendWidth     = 0.15f;  // smoothstep transition between terracing
        float cliffStep           = 12.0f;  // terrace height = vertical face size in blocks
        float mesaBandThickness   = 2.0f;   // blocks per color band
        float mesaBandJitterScale = 60.0f;  // wavelength of the band edge wobble
        float mesaBandJitterAmp   = 0.f;    // blocks the band line wobbles
        float cliffRiser          = 0.25f;  // riser width, small = sharp cliff
        float cliffPhaseScale     = 300.0f; // wavelength of the region terrace phase offset

        // WATER & RIVERS ----------------------------------------------------------------------------------------------
        int  waterLevel              = 58;     // water surface height
        bool iceEnabled              = false;  // freeze rivers in cold biomes
        bool taigaRivers             = true;   // allow rivers through taiga
        // River path
        float riverScale             = 320.0f; // meander wavelength (large = weavier rivers)
        int   riverNoiseOct          = 1;      // keep at 1, else unstable
        float riverHalfWidth         = 12.0f;  // river half-width in blocks
        float riverGradMin           = 0.0f;   // cull flat spots in fields
        bool  riverWarpEnabled       = true;   // meander warp
        float riverWarpScale         = 110.f;
        float riverWarpAmp           = 15.0f;
        // River channel
        int   riverLevel             = 64;     // flat river surface height
        int   riverDepth             = 7;      // deep channel depth
        int   riverShelfDepth        = 3;      // shallow outer shelf depth
        float channelThreshold       = 0.525f; // valleyTerr above this carves the deep channel
        float riverBankSharpnessMtn  = 1.0f;   // for mountains, smoothes river bore entrance
        float riverBankSharpnessMesa = 1.5f;   // same as above but for mesas
        // River reach
        int   riverMaxHeight         = 120;    // Y where open rivers fade out
        float riverFade              = 12.f;   // fade range in blocks
        // River banks
        float beachBand              = 0.20f;  // how far past the water edge the beach reaches
        float beachSandChance        = 0.70f;  // sand vs surface scatter
        bool  desertRiverGrass       = true;   // desert river banks scatter grass instead of sand

        // RIVER TUNNELS -----------------------------------------------------------------------------------------------
        bool  riverTunnels         = true;   // rivers bore through mountains and mesas
        float tunnelMaskThreshMtn  = 0.20f;  // only solid mountain interiors bore
        float tunnelMaskThreshMesa = 0.05f;  // same as before for mesas
        int   tunnelRiseMtn        = 12;     // Amout of blocks above river to start boring
        int   tunnelRiseMesa       = 32;     // same as above for mesas
        float riverArchHeight      = 10.0f;  // max arched void height above the water at the river centre
        float riverCeilScale       = 22.0f;  // ceiling roughness wavelength, low thighter
        float riverCeilJitter      = 7.0f;   // blocks the rock ceiling dips for irregularity
        float calciteChance        = 0.55f;  // ceiling formation scatter density
        int   calciteBand          = 3;      // blocks above the ceiling that can turn to formation rock
        float dripstoneFraction    = 0.7f;   // formation split, currently mostly dripstone, percentage

        // STRATA & ORES -----------------------------------------------------------------------------------------------
        float strataScale   = 22.0f;   // diorite/granite clump size
        float dioriteThresh = 0.78f;
        float graniteThresh = 0.78f;
        float oreScale      = 9.0f;    // ore clump size
        float coalThresh    = 0.82f;
        float ironThresh    = 0.88f;
        float copperThresh  = 0.85f;

        // LOD SETTINGS  -----------------------------------------------------------------------------------------------
        float lodCanopyCoverage     = 0.5f;
        int   proxyMaxLevel         = 2;     // canopy beyong this ring
        float proxyKeepBase         = 1.0f;  // match core desnity
        float proxyKeepFalloff      = 0.4f;
        int   lodRiverCarveMaxLevel = 2;    // carve rivers up to this LOD, after is coloration only
        float lodRiverWetProfile    = 0.5f;
        float lodRiverPaintThresh   = 0.5f;
        int   lodBandMaxLevel       = 2;

        // AGGREGATION SETTINGS ----------------------------------------------------------------------------------------
        int biomeStrideLevel = 4;

    };
}
