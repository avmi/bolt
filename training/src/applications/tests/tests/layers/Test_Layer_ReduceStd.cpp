// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <tests/tools/TestTools.h>

#include <training/layers/basic/DataLayer.h>
#include <training/layers/basic/ReduceStdLayer.h>
#include <training/network/Workflow.h>

namespace UT
{

// See reduce_std.py
TEST(TestLayerReduceStd, ForwardUnit)
{
    PROFILE_TEST
    // Test parameters
    const auto batch = 2;
    const auto depth = 3;
    const auto height = 4;
    const auto width = 5;
    const auto eps = TODTYPE(1e-6);

    // See reduce_mean.py
    const raul::Tensor x{ 0.49625659_dt, 0.76822180_dt, 0.08847743_dt, 0.13203049_dt, 0.30742282_dt, 0.63407868_dt, 0.49009341_dt, 0.89644474_dt, 0.45562798_dt, 0.63230628_dt, 0.34889346_dt,
                          0.40171731_dt, 0.02232575_dt, 0.16885895_dt, 0.29388845_dt, 0.51852179_dt, 0.69766760_dt, 0.80001140_dt, 0.16102946_dt, 0.28226858_dt, 0.68160856_dt, 0.91519397_dt,
                          0.39709991_dt, 0.87415588_dt, 0.41940832_dt, 0.55290705_dt, 0.95273811_dt, 0.03616482_dt, 0.18523103_dt, 0.37341738_dt, 0.30510002_dt, 0.93200040_dt, 0.17591017_dt,
                          0.26983356_dt, 0.15067977_dt, 0.03171951_dt, 0.20812976_dt, 0.92979902_dt, 0.72310919_dt, 0.74233627_dt, 0.52629578_dt, 0.24365824_dt, 0.58459234_dt, 0.03315264_dt,
                          0.13871688_dt, 0.24223500_dt, 0.81546897_dt, 0.79316062_dt, 0.27825248_dt, 0.48195881_dt, 0.81978035_dt, 0.99706656_dt, 0.69844109_dt, 0.56754643_dt, 0.83524317_dt,
                          0.20559883_dt, 0.59317201_dt, 0.11234725_dt, 0.15345693_dt, 0.24170822_dt, 0.72623652_dt, 0.70108020_dt, 0.20382375_dt, 0.65105355_dt, 0.77448601_dt, 0.43689132_dt,
                          0.51909077_dt, 0.61585236_dt, 0.81018829_dt, 0.98009706_dt, 0.11468822_dt, 0.31676513_dt, 0.69650495_dt, 0.91427469_dt, 0.93510365_dt, 0.94117838_dt, 0.59950727_dt,
                          0.06520867_dt, 0.54599625_dt, 0.18719733_dt, 0.03402293_dt, 0.94424623_dt, 0.88017988_dt, 0.00123602_dt, 0.59358603_dt, 0.41576999_dt, 0.41771942_dt, 0.27112156_dt,
                          0.69227809_dt, 0.20384824_dt, 0.68329567_dt, 0.75285405_dt, 0.85793579_dt, 0.68695557_dt, 0.00513238_dt, 0.17565155_dt, 0.74965751_dt, 0.60465068_dt, 0.10995799_dt,
                          0.21209025_dt, 0.97037464_dt, 0.83690894_dt, 0.28198743_dt, 0.37415761_dt, 0.02370095_dt, 0.49101293_dt, 0.12347054_dt, 0.11432165_dt, 0.47245020_dt, 0.57507253_dt,
                          0.29523486_dt, 0.79668880_dt, 0.19573045_dt, 0.95368505_dt, 0.84264994_dt, 0.07835853_dt, 0.37555784_dt, 0.52256131_dt, 0.57295054_dt, 0.61858714_dt };

    std::string dimensions[] = { "default", "batch", "depth", "height", "width" };

    raul::Tensor realOutputs[] = {
        { 0.29362151_dt },
        { 0.16262037_dt, 0.04747628_dt, 0.08156216_dt, 0.36700472_dt, 0.33026356_dt, 0.13943252_dt, 0.02050423_dt, 0.19840878_dt, 0.25071201_dt, 0.24592522_dt, 0.16560812_dt, 0.06007026_dt,
          0.47671667_dt, 0.52708852_dt, 0.45340762_dt, 0.29886335_dt, 0.06940983_dt, 0.51958400_dt, 0.27221262_dt, 0.06722553_dt, 0.45791218_dt, 0.02054305_dt, 0.34158912_dt, 0.61724752_dt,
          0.12316224_dt, 0.09697054_dt, 0.37831533_dt, 0.16613950_dt, 0.35853642_dt, 0.11990348_dt, 0.26742470_dt, 0.12667561_dt, 0.48226494_dt, 0.29494980_dt, 0.10291754_dt, 0.10177533_dt,
          0.38291794_dt, 0.22991461_dt, 0.43356335_dt, 0.37494054_dt, 0.31401119_dt, 0.41949159_dt, 0.21397398_dt, 0.24112692_dt, 0.08132854_dt, 0.17591256_dt, 0.48931679_dt, 0.48001164_dt,
          0.13731852_dt, 0.06584134_dt, 0.37090966_dt, 0.14168848_dt, 0.35547009_dt, 0.27304125_dt, 0.00523738_dt, 0.08997248_dt, 0.15387645_dt, 0.29006514_dt, 0.29662678_dt, 0.26649365_dt },
        { 0.09948179_dt, 0.35301745_dt, 0.25050989_dt, 0.45967624_dt, 0.14129764_dt, 0.20682015_dt, 0.23761038_dt, 0.46971476_dt, 0.13737392_dt, 0.13000581_dt,
          0.28535011_dt, 0.32656661_dt, 0.35443822_dt, 0.20727620_dt, 0.36106342_dt, 0.24668849_dt, 0.25781912_dt, 0.43930897_dt, 0.32672483_dt, 0.27806950_dt,
          0.48571557_dt, 0.12186089_dt, 0.37000030_dt, 0.32608911_dt, 0.39182734_dt, 0.03880884_dt, 0.20549692_dt, 0.25656661_dt, 0.17141283_dt, 0.38824704_dt,
          0.29054624_dt, 0.26533681_dt, 0.34528995_dt, 0.14397441_dt, 0.51231986_dt, 0.47257370_dt, 0.18825914_dt, 0.29066241_dt, 0.25987753_dt, 0.24219708_dt },
        { 0.11714106_dt, 0.17203265_dt, 0.46022153_dt, 0.15165715_dt, 0.16920234_dt, 0.28700468_dt, 0.36291552_dt, 0.39257979_dt, 0.33721498_dt, 0.24399167_dt,
          0.28603467_dt, 0.32432956_dt, 0.30213991_dt, 0.22928490_dt, 0.30935216_dt, 0.35883123_dt, 0.16291125_dt, 0.30821595_dt, 0.16378510_dt, 0.36549741_dt,
          0.28493771_dt, 0.21875301_dt, 0.28388804_dt, 0.36873907_dt, 0.24598438_dt, 0.38045391_dt, 0.34370273_dt, 0.17643066_dt, 0.25358731_dt, 0.34788322_dt },
        { 0.28002605_dt, 0.17367399_dt, 0.15256368_dt, 0.26990846_dt, 0.24414290_dt, 0.35570002_dt, 0.32241577_dt, 0.38537532_dt, 0.24106906_dt, 0.27337542_dt, 0.16083780_dt, 0.19199124_dt,
          0.23211947_dt, 0.22126274_dt, 0.36606798_dt, 0.34941602_dt, 0.45169494_dt, 0.18779923_dt, 0.33846119_dt, 0.28702292_dt, 0.39526996_dt, 0.21923594_dt, 0.34552982_dt, 0.21858540_dt }
    };

    for (size_t iter = 1; iter < std::size(dimensions); ++iter)
    {
        MANAGERS_DEFINE
        NETWORK_PARAMS_DEFINE(networkParameters);

        work.add<raul::DataLayer>("data", raul::DataParams{ { "x" }, depth, height, width });
        work.add<raul::ReduceStdLayer>("rstd", raul::BasicParamsWithDim{ { "x" }, { "out" }, dimensions[iter] });
        TENSORS_CREATE(batch);
        memory_manager["x"] = TORANGE(x);

        work.forwardPassTraining();

        // Checks
        const auto& outTensor = memory_manager["out"];
        EXPECT_EQ(outTensor.size(), realOutputs[iter].size());
        for (size_t i = 0; i < outTensor.size(); ++i)
        {
            ASSERT_TRUE(tools::expect_near_relative(outTensor[i], realOutputs[iter][i], eps));
        }
    }
}

TEST(TestLayerReduceStd, BackwardUnit)
{
    PROFILE_TEST
    // Test parameters
    const auto batch = 2;
    const auto depth = 3;
    const auto height = 4;
    const auto width = 5;
    const auto eps = TODTYPE(1e-4);

    const raul::Tensor x{ 0.49625659_dt, 0.76822180_dt, 0.08847743_dt, 0.13203049_dt, 0.30742282_dt, 0.63407868_dt, 0.49009341_dt, 0.89644474_dt, 0.45562798_dt, 0.63230628_dt, 0.34889346_dt,
                          0.40171731_dt, 0.02232575_dt, 0.16885895_dt, 0.29388845_dt, 0.51852179_dt, 0.69766760_dt, 0.80001140_dt, 0.16102946_dt, 0.28226858_dt, 0.68160856_dt, 0.91519397_dt,
                          0.39709991_dt, 0.87415588_dt, 0.41940832_dt, 0.55290705_dt, 0.95273811_dt, 0.03616482_dt, 0.18523103_dt, 0.37341738_dt, 0.30510002_dt, 0.93200040_dt, 0.17591017_dt,
                          0.26983356_dt, 0.15067977_dt, 0.03171951_dt, 0.20812976_dt, 0.92979902_dt, 0.72310919_dt, 0.74233627_dt, 0.52629578_dt, 0.24365824_dt, 0.58459234_dt, 0.03315264_dt,
                          0.13871688_dt, 0.24223500_dt, 0.81546897_dt, 0.79316062_dt, 0.27825248_dt, 0.48195881_dt, 0.81978035_dt, 0.99706656_dt, 0.69844109_dt, 0.56754643_dt, 0.83524317_dt,
                          0.20559883_dt, 0.59317201_dt, 0.11234725_dt, 0.15345693_dt, 0.24170822_dt, 0.72623652_dt, 0.70108020_dt, 0.20382375_dt, 0.65105355_dt, 0.77448601_dt, 0.43689132_dt,
                          0.51909077_dt, 0.61585236_dt, 0.81018829_dt, 0.98009706_dt, 0.11468822_dt, 0.31676513_dt, 0.69650495_dt, 0.91427469_dt, 0.93510365_dt, 0.94117838_dt, 0.59950727_dt,
                          0.06520867_dt, 0.54599625_dt, 0.18719733_dt, 0.03402293_dt, 0.94424623_dt, 0.88017988_dt, 0.00123602_dt, 0.59358603_dt, 0.41576999_dt, 0.41771942_dt, 0.27112156_dt,
                          0.69227809_dt, 0.20384824_dt, 0.68329567_dt, 0.75285405_dt, 0.85793579_dt, 0.68695557_dt, 0.00513238_dt, 0.17565155_dt, 0.74965751_dt, 0.60465068_dt, 0.10995799_dt,
                          0.21209025_dt, 0.97037464_dt, 0.83690894_dt, 0.28198743_dt, 0.37415761_dt, 0.02370095_dt, 0.49101293_dt, 0.12347054_dt, 0.11432165_dt, 0.47245020_dt, 0.57507253_dt,
                          0.29523486_dt, 0.79668880_dt, 0.19573045_dt, 0.95368505_dt, 0.84264994_dt, 0.07835853_dt, 0.37555784_dt, 0.52256131_dt, 0.57295054_dt, 0.61858714_dt };

    // Always one
    const raul::Tensor realGrads[] = {
        { 0.00028462_dt,  0.00806818_dt,  -0.01138590_dt, -0.01013943_dt, -0.00511975_dt, 0.00422904_dt,  0.00010823_dt,  0.01173788_dt,  -0.00087816_dt, 0.00417832_dt,  -0.00393287_dt,
          -0.00242107_dt, -0.01327915_dt, -0.00908541_dt, -0.00550710_dt, 0.00092184_dt,  0.00604894_dt,  0.00897799_dt,  -0.00930949_dt, -0.00583966_dt, 0.00558934_dt,  0.01227448_dt,
          -0.00255322_dt, 0.01109998_dt,  -0.00191476_dt, 0.00190594_dt,  0.01334898_dt,  -0.01288308_dt, -0.00861684_dt, -0.00323101_dt, -0.00518623_dt, 0.01275548_dt,  -0.00888360_dt,
          -0.00619554_dt, -0.00960569_dt, -0.01301030_dt, -0.00796149_dt, 0.01269247_dt,  0.00677707_dt,  0.00732735_dt,  0.00114433_dt,  -0.00694467_dt, 0.00281276_dt,  -0.01296928_dt,
          -0.00994807_dt, -0.00698541_dt, 0.00942038_dt,  0.00878192_dt,  -0.00595460_dt, -0.00012458_dt, 0.00954377_dt,  0.01461765_dt,  0.00607108_dt,  0.00232491_dt,  0.00998631_dt,
          -0.00803392_dt, 0.00305831_dt,  -0.01070276_dt, -0.00952621_dt, -0.00700048_dt, 0.00686657_dt,  0.00614661_dt,  -0.00808473_dt, 0.00471486_dt,  0.00824746_dt,  -0.00141440_dt,
          0.00093812_dt,  0.00370741_dt,  0.00926925_dt,  0.01413199_dt,  -0.01063576_dt, -0.00485238_dt, 0.00601567_dt,  0.01224817_dt,  0.01284429_dt,  0.01301815_dt,  0.00323962_dt,
          -0.01205185_dt, 0.00170815_dt,  -0.00856057_dt, -0.01294438_dt, 0.01310595_dt,  0.01127239_dt,  -0.01388273_dt, 0.00307016_dt,  -0.00201889_dt, -0.00196309_dt, -0.00615868_dt,
          0.00589469_dt,  -0.00808403_dt, 0.00563762_dt,  0.00762836_dt,  0.01063577_dt,  0.00574237_dt,  -0.01377122_dt, -0.00889101_dt, 0.00753688_dt,  0.00338682_dt,  -0.01077114_dt,
          -0.00784814_dt, 0.01385374_dt,  0.01003399_dt,  -0.00584770_dt, -0.00320982_dt, -0.01323979_dt, 0.00013454_dt,  -0.01038441_dt, -0.01064625_dt, -0.00039672_dt, 0.00254031_dt,
          -0.00546857_dt, 0.00888290_dt,  -0.00831635_dt, 0.01337609_dt,  0.01019829_dt,  -0.01167550_dt, -0.00316975_dt, 0.00103745_dt,  0.00247957_dt,  0.00378568_dt },
        { -0.70710701_dt, 0.70710737_dt,  -0.70710677_dt, -0.70710677_dt, -0.70710677_dt, 0.70710659_dt,  -0.70710534_dt, 0.70710665_dt,  -0.70710665_dt, -0.70710689_dt, 0.70710677_dt,
          0.70710677_dt,  -0.70710677_dt, -0.70710683_dt, -0.70710677_dt, -0.70710665_dt, 0.70710635_dt,  0.70710677_dt,  -0.70710683_dt, 0.70710677_dt,  0.70710677_dt,  -0.70710677_dt,
          -0.70710683_dt, 0.70710683_dt,  -0.70710653_dt, 0.70710677_dt,  0.70710683_dt,  -0.70710683_dt, -0.70710677_dt, 0.70710677_dt,  -0.70710677_dt, 0.70710653_dt,  -0.70710671_dt,
          -0.70710683_dt, 0.70710683_dt,  -0.70710677_dt, -0.70710677_dt, 0.70710677_dt,  0.70710683_dt,  0.70710677_dt,  -0.70710683_dt, -0.70710671_dt, 0.70710677_dt,  -0.70710677_dt,
          0.70710677_dt,  -0.70710677_dt, 0.70710677_dt,  0.70710677_dt,  -0.70710677_dt, -0.70710731_dt, 0.70710671_dt,  0.70710695_dt,  0.70710677_dt,  -0.70710665_dt, -0.70711243_dt,
          0.70710677_dt,  0.70710683_dt,  -0.70710677_dt, -0.70710683_dt, -0.70710677_dt, 0.70710665_dt,  -0.70710611_dt, 0.70710677_dt,  0.70710677_dt,  0.70710677_dt,  -0.70710701_dt,
          0.70710826_dt,  -0.70710695_dt, 0.70710689_dt,  0.70710665_dt,  -0.70710677_dt, -0.70710677_dt, 0.70710677_dt,  0.70710671_dt,  0.70710677_dt,  0.70710683_dt,  -0.70710719_dt,
          -0.70710677_dt, 0.70710683_dt,  -0.70710677_dt, -0.70710677_dt, 0.70710677_dt,  0.70710665_dt,  -0.70710683_dt, 0.70710701_dt,  -0.70710677_dt, -0.70710683_dt, 0.70710683_dt,
          0.70710677_dt,  -0.70710677_dt, 0.70710677_dt,  -0.70710701_dt, 0.70710683_dt,  0.70710683_dt,  -0.70710683_dt, 0.70710677_dt,  0.70710677_dt,  -0.70710677_dt, -0.70710683_dt,
          -0.70710677_dt, 0.70710665_dt,  0.70710683_dt,  -0.70710677_dt, 0.70710677_dt,  -0.70710677_dt, 0.70710677_dt,  -0.70710677_dt, -0.70710677_dt, 0.70710677_dt,  0.70710635_dt,
          -0.70710689_dt, -0.70710653_dt, -0.70710677_dt, 0.70710689_dt,  0.70710105_dt,  -0.70710677_dt, -0.70710683_dt, 0.70710677_dt,  0.70710683_dt,  0.70710677_dt },
        { -0.36085534_dt, 0.17826852_dt,  -0.53539962_dt, -0.23322488_dt, 0.06690416_dt,  0.38118088_dt,  -0.55273986_dt, 0.34189680_dt,  0.54325259_dt,  0.52463841_dt,  -0.24945578_dt,
          -0.57447821_dt, -0.39014784_dt, -0.40176836_dt, -0.18378398_dt, 0.54030710_dt,  0.38401178_dt,  0.21164922_dt,  -0.28286144_dt, -0.25144035_dt, 0.57073206_dt,  0.38643411_dt,
          0.08058902_dt,  0.57400125_dt,  0.46317938_dt,  0.18494365_dt,  0.42079657_dt,  -0.57385033_dt, -0.44091141_dt, -0.47104365_dt, -0.32619211_dt, 0.23742823_dt,  -0.17348889_dt,
          -0.15819331_dt, -0.38209912_dt, -0.44636706_dt, -0.56537056_dt, 0.35936716_dt,  0.57731158_dt,  0.57581276_dt,  -0.20987700_dt, -0.56470263_dt, 0.45481065_dt,  -0.34077650_dt,
          -0.53008366_dt, -0.56612444_dt, 0.13194315_dt,  0.23195337_dt,  -0.10234116_dt, -0.05359525_dt, 0.57564795_dt,  0.33704981_dt,  0.56363666_dt,  0.55996168_dt,  0.56588310_dt,
          -0.09394012_dt, 0.18135889_dt,  -0.57101655_dt, -0.29444999_dt, -0.32437241_dt, 0.15375100_dt,  -0.51834363_dt, -0.33987352_dt, 0.47365043_dt,  0.39629915_dt,  -0.14172114_dt,
          0.40308106_dt,  0.54973477_dt,  0.44303191_dt,  0.50709689_dt,  -0.42973891_dt, -0.57537729_dt, 0.16379645_dt,  0.21752545_dt,  0.33261284_dt,  0.57428318_dt,  0.06533476_dt,
          -0.57156497_dt, 0.26235691_dt,  -0.31398860_dt, -0.55881995_dt, 0.47937614_dt,  0.57412046_dt,  -0.52272981_dt, 0.16545771_dt,  -0.41384110_dt, 0.15643176_dt,  -0.12208061_dt,
          0.09909556_dt,  -0.49258718_dt, 0.54877567_dt,  0.24638754_dt,  0.39755774_dt,  -0.57191736_dt, -0.57499522_dt, -0.23567180_dt, 0.46412084_dt,  0.35638785_dt,  -0.57657337_dt,
          -0.26259878_dt, 0.40506899_dt,  0.03896700_dt,  -0.23424701_dt, 0.04907946_dt,  -0.56175685_dt, 0.55556339_dt,  -0.55951297_dt, -0.42765412_dt, -0.54212797_dt, -0.01450979_dt,
          -0.11903682_dt, 0.32898962_dt,  -0.56135428_dt, 0.35439128_dt,  0.24238238_dt,  -0.33861133_dt, -0.52945560_dt, 0.21517701_dt,  0.31421652_dt,  0.57658738_dt },
        { -0.00905188_dt, 0.34643960_dt,  -0.26316124_dt, -0.21398319_dt, -0.14095294_dt, 0.38313085_dt,  -0.19246660_dt, 0.32204050_dt,  0.49726480_dt,  0.49907655_dt,  -0.42838430_dt,
          -0.36370561_dt, -0.31107417_dt, -0.13303643_dt, -0.16761602_dt, 0.05430534_dt,  0.20973262_dt,  0.25219491_dt,  -0.15024517_dt, -0.19050747_dt, 0.33538920_dt,  0.14987729_dt,
          0.01049165_dt,  0.35691717_dt,  -0.00280355_dt, 0.18591252_dt,  0.18436110_dt,  -0.29597268_dt, -0.32407758_dt, -0.06563485_dt, -0.10189584_dt, 0.16531378_dt,  -0.17731708_dt,
          -0.24044888_dt, -0.36993161_dt, -0.41940579_dt, -0.49955231_dt, 0.46279809_dt,  0.20760916_dt,  0.43836996_dt,  0.09068630_dt,  -0.43030638_dt, 0.04132411_dt,  -0.32703054_dt,
          -0.30783674_dt, -0.24034676_dt, 0.15737849_dt,  0.27142534_dt,  0.02929451_dt,  0.06201351_dt,  0.43270147_dt,  0.34401745_dt,  0.16692679_dt,  0.44986871_dt,  0.44268468_dt,
          -0.28304109_dt, -0.07108969_dt, -0.47967637_dt, -0.15213270_dt, -0.19686145_dt, 0.15930231_dt,  0.34163666_dt,  -0.20713149_dt, -0.16144042_dt, 0.05040164_dt,  -0.10948256_dt,
          -0.03073263_dt, 0.23847453_dt,  0.16242859_dt,  0.23791875_dt,  -0.40879050_dt, -0.44471192_dt, 0.32569975_dt,  0.37426388_dt,  0.19688481_dt,  0.35897079_dt,  0.13380790_dt,
          -0.35704270_dt, -0.37525168_dt, -0.48520514_dt, -0.34295461_dt, 0.34761727_dt,  0.26619399_dt,  -0.33571249_dt, 0.46062791_dt,  0.10363079_dt,  -0.45469835_dt, -0.44894511_dt,
          0.28897685_dt,  -0.06750562_dt, 0.41659471_dt,  0.05597601_dt,  0.24007560_dt,  0.28416538_dt,  -0.33678544_dt, -0.17727089_dt, 0.05110516_dt,  -0.05732463_dt, -0.23742981_dt,
          -0.05633688_dt, 0.44826221_dt,  0.29458833_dt,  0.00630504_dt,  -0.28807071_dt, -0.47075340_dt, 0.02827119_dt,  -0.39732590_dt, -0.31046867_dt, -0.15886790_dt, 0.05755755_dt,
          -0.14325906_dt, 0.25558162_dt,  -0.15666170_dt, 0.47370172_dt,  0.31394377_dt,  -0.33327448_dt, -0.15284398_dt, 0.46082523_dt,  -0.02676303_dt, 0.09925220_dt },
        { 0.12300174_dt,  0.36580524_dt,  -0.24105293_dt, -0.20216990_dt, -0.04558416_dt, 0.01780419_dt,  -0.18945953_dt, 0.39547452_dt,  -0.23907179_dt, 0.01525285_dt,  0.16674460_dt,
          0.25330493_dt,  -0.36838886_dt, -0.12827078_dt, 0.07661008_dt,  0.02465836_dt,  0.19059037_dt,  0.28538528_dt,  -0.30646533_dt, -0.19416881_dt, 0.02469380_dt,  0.26388302_dt,
          -0.26664034_dt, 0.22186041_dt,  -0.24379672_dt, 0.09334787_dt,  0.37436491_dt,  -0.26983893_dt, -0.16506933_dt, -0.03280455_dt, -0.04776811_dt, 0.43832812_dt,  -0.14794146_dt,
          -0.07511359_dt, -0.16750501_dt, -0.32130963_dt, -0.20686908_dt, 0.26129094_dt,  0.12720746_dt,  0.13968042_dt,  0.22920054_dt,  -0.06390795_dt, 0.28965682_dt,  -0.28221220_dt,
          -0.17273711_dt, -0.25604001_dt, 0.26817861_dt,  0.24777779_dt,  -0.22310227_dt, -0.03681419_dt, 0.05621328_dt,  0.33178005_dt,  -0.13239174_dt, -0.33584929_dt, 0.08024807_dt,
          -0.07247441_dt, 0.43220121_dt,  -0.19390129_dt, -0.14037061_dt, -0.02545483_dt, 0.12375144_dt,  0.09665728_dt,  -0.43890363_dt, 0.04277700_dt,  0.17571765_dt,  -0.26612324_dt,
          -0.17324786_dt, -0.06391903_dt, 0.15565695_dt,  0.34763318_dt,  -0.32834005_dt, -0.19033501_dt, 0.06900195_dt,  0.21772416_dt,  0.23194894_dt,  0.33867994_dt,  0.09422126_dt,
          -0.28805843_dt, 0.05593522_dt,  -0.20077805_dt, -0.25273210_dt, 0.25105003_dt,  0.21559115_dt,  -0.27087870_dt, 0.05696979_dt,  0.02079688_dt,  0.02339198_dt,  -0.17176038_dt,
          0.38888690_dt,  -0.26131523_dt, 0.06356785_dt,  0.11494626_dt,  0.19256353_dt,  0.06627119_dt,  -0.43734875_dt, -0.16962932_dt, 0.33033594_dt,  0.20403346_dt,  -0.22684911_dt,
          -0.13789083_dt, 0.29913017_dt,  0.21471591_dt,  -0.13626036_dt, -0.07796465_dt, -0.29962116_dt, 0.15479599_dt,  -0.26432142_dt, -0.27475411_dt, 0.13362846_dt,  0.25065115_dt,
          -0.23265933_dt, 0.13015591_dt,  -0.30465341_dt, 0.24374686_dt,  0.16341001_dt,  -0.40629953_dt, -0.06638741_dt, 0.10174308_dt,  0.15937413_dt,  0.21156952_dt }
    };

    std::string dimensions[] = { "default", "batch", "depth", "height", "width" };

    for (size_t iter = 0; iter < std::size(dimensions); ++iter)
    {
        MANAGERS_DEFINE
        NETWORK_PARAMS_DEFINE(networkParameters);

        work.add<raul::DataLayer>("data", raul::DataParams{ { "x" }, depth, height, width });
        work.add<raul::ReduceStdLayer>("rstd", raul::BasicParamsWithDim{ { "x" }, { "out" }, dimensions[iter] });
        TENSORS_CREATE(batch);
        memory_manager["x"] = TORANGE(x);
        memory_manager[raul::Name("out").grad()] = 1.0_dt;

        work.forwardPassTraining();
        work.backwardPassTraining();

        // Checks
        const auto& x_tensor_grad = memory_manager[raul::Name("x").grad()];
        EXPECT_EQ(x_tensor_grad.getShape(), memory_manager["x"].getShape());
        for (size_t i = 0; i < x_tensor_grad.size(); ++i)
        {
            ASSERT_TRUE(tools::expect_near_relative(x_tensor_grad[i], realGrads[iter][i], eps));
        }
    }
}

}