/**
 * rijndael-alg-fst.c
 *
 * @version 3.0 (December 2000)
 *
 * Optimised ANSI C code for the Rijndael cipher (now AES)
 *
 * @author Vincent Rijmen <vincent.rijmen@esat.kuleuven.ac.be>
 * @author Antoon Bosselaers <antoon.bosselaers@esat.kuleuven.ac.be>
 * @author Paulo Barreto <paulo.barreto@terra.com.br>
 *
 * This code is hereby placed in the public domain.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
#include "crypto/rijndael-alg-fst.h" // @v9.5.5
/*
   Te0[x] = S [x].[02, 01, 01, 03];
   Te1[x] = S [x].[03, 02, 01, 01];
   Te2[x] = S [x].[01, 03, 02, 01];
   Te3[x] = S [x].[01, 01, 03, 02];
   Te4[x] = S [x].[01, 01, 01, 01];

   Td0[x] = Si[x].[0e, 09, 0d, 0b];
   Td1[x] = Si[x].[0b, 0e, 09, 0d];
   Td2[x] = Si[x].[0d, 0b, 0e, 09];
   Td3[x] = Si[x].[09, 0d, 0b, 0e];
   Td4[x] = Si[x].[01, 01, 01, 01];
 */

static const u32 Te0[256] = {
	(uint)0xc66363a5, (uint)0xf87c7c84, (uint)0xee777799, (uint)0xf67b7b8d,
	(uint)0xfff2f20d, (uint)0xd66b6bbd, (uint)0xde6f6fb1, (uint)0x91c5c554,
	(uint)0x60303050, (uint)0x02010103, (uint)0xce6767a9, (uint)0x562b2b7d,
	(uint)0xe7fefe19, (uint)0xb5d7d762, (uint)0x4dababe6, (uint)0xec76769a,
	(uint)0x8fcaca45, (uint)0x1f82829d, (uint)0x89c9c940, (uint)0xfa7d7d87,
	(uint)0xeffafa15, (uint)0xb25959eb, (uint)0x8e4747c9, (uint)0xfbf0f00b,
	(uint)0x41adadec, (uint)0xb3d4d467, (uint)0x5fa2a2fd, (uint)0x45afafea,
	(uint)0x239c9cbf, (uint)0x53a4a4f7, (uint)0xe4727296, (uint)0x9bc0c05b,
	(uint)0x75b7b7c2, (uint)0xe1fdfd1c, (uint)0x3d9393ae, (uint)0x4c26266a,
	(uint)0x6c36365a, (uint)0x7e3f3f41, (uint)0xf5f7f702, (uint)0x83cccc4f,
	(uint)0x6834345c, (uint)0x51a5a5f4, (uint)0xd1e5e534, (uint)0xf9f1f108,
	(uint)0xe2717193, (uint)0xabd8d873, (uint)0x62313153, (uint)0x2a15153f,
	(uint)0x0804040c, (uint)0x95c7c752, (uint)0x46232365, (uint)0x9dc3c35e,
	(uint)0x30181828, (uint)0x379696a1, (uint)0x0a05050f, (uint)0x2f9a9ab5,
	(uint)0x0e070709, (uint)0x24121236, (uint)0x1b80809b, (uint)0xdfe2e23d,
	(uint)0xcdebeb26, (uint)0x4e272769, (uint)0x7fb2b2cd, (uint)0xea75759f,
	(uint)0x1209091b, (uint)0x1d83839e, (uint)0x582c2c74, (uint)0x341a1a2e,
	(uint)0x361b1b2d, (uint)0xdc6e6eb2, (uint)0xb45a5aee, (uint)0x5ba0a0fb,
	(uint)0xa45252f6, (uint)0x763b3b4d, (uint)0xb7d6d661, (uint)0x7db3b3ce,
	(uint)0x5229297b, (uint)0xdde3e33e, (uint)0x5e2f2f71, (uint)0x13848497,
	(uint)0xa65353f5, (uint)0xb9d1d168, (uint)0x00000000, (uint)0xc1eded2c,
	(uint)0x40202060, (uint)0xe3fcfc1f, (uint)0x79b1b1c8, (uint)0xb65b5bed,
	(uint)0xd46a6abe, (uint)0x8dcbcb46, (uint)0x67bebed9, (uint)0x7239394b,
	(uint)0x944a4ade, (uint)0x984c4cd4, (uint)0xb05858e8, (uint)0x85cfcf4a,
	(uint)0xbbd0d06b, (uint)0xc5efef2a, (uint)0x4faaaae5, (uint)0xedfbfb16,
	(uint)0x864343c5, (uint)0x9a4d4dd7, (uint)0x66333355, (uint)0x11858594,
	(uint)0x8a4545cf, (uint)0xe9f9f910, (uint)0x04020206, (uint)0xfe7f7f81,
	(uint)0xa05050f0, (uint)0x783c3c44, (uint)0x259f9fba, (uint)0x4ba8a8e3,
	(uint)0xa25151f3, (uint)0x5da3a3fe, (uint)0x804040c0, (uint)0x058f8f8a,
	(uint)0x3f9292ad, (uint)0x219d9dbc, (uint)0x70383848, (uint)0xf1f5f504,
	(uint)0x63bcbcdf, (uint)0x77b6b6c1, (uint)0xafdada75, (uint)0x42212163,
	(uint)0x20101030, (uint)0xe5ffff1a, (uint)0xfdf3f30e, (uint)0xbfd2d26d,
	(uint)0x81cdcd4c, (uint)0x180c0c14, (uint)0x26131335, (uint)0xc3ecec2f,
	(uint)0xbe5f5fe1, (uint)0x359797a2, (uint)0x884444cc, (uint)0x2e171739,
	(uint)0x93c4c457, (uint)0x55a7a7f2, (uint)0xfc7e7e82, (uint)0x7a3d3d47,
	(uint)0xc86464ac, (uint)0xba5d5de7, (uint)0x3219192b, (uint)0xe6737395,
	(uint)0xc06060a0, (uint)0x19818198, (uint)0x9e4f4fd1, (uint)0xa3dcdc7f,
	(uint)0x44222266, (uint)0x542a2a7e, (uint)0x3b9090ab, (uint)0x0b888883,
	(uint)0x8c4646ca, (uint)0xc7eeee29, (uint)0x6bb8b8d3, (uint)0x2814143c,
	(uint)0xa7dede79, (uint)0xbc5e5ee2, (uint)0x160b0b1d, (uint)0xaddbdb76,
	(uint)0xdbe0e03b, (uint)0x64323256, (uint)0x743a3a4e, (uint)0x140a0a1e,
	(uint)0x924949db, (uint)0x0c06060a, (uint)0x4824246c, (uint)0xb85c5ce4,
	(uint)0x9fc2c25d, (uint)0xbdd3d36e, (uint)0x43acacef, (uint)0xc46262a6,
	(uint)0x399191a8, (uint)0x319595a4, (uint)0xd3e4e437, (uint)0xf279798b,
	(uint)0xd5e7e732, (uint)0x8bc8c843, (uint)0x6e373759, (uint)0xda6d6db7,
	(uint)0x018d8d8c, (uint)0xb1d5d564, (uint)0x9c4e4ed2, (uint)0x49a9a9e0,
	(uint)0xd86c6cb4, (uint)0xac5656fa, (uint)0xf3f4f407, (uint)0xcfeaea25,
	(uint)0xca6565af, (uint)0xf47a7a8e, (uint)0x47aeaee9, (uint)0x10080818,
	(uint)0x6fbabad5, (uint)0xf0787888, (uint)0x4a25256f, (uint)0x5c2e2e72,
	(uint)0x381c1c24, (uint)0x57a6a6f1, (uint)0x73b4b4c7, (uint)0x97c6c651,
	(uint)0xcbe8e823, (uint)0xa1dddd7c, (uint)0xe874749c, (uint)0x3e1f1f21,
	(uint)0x964b4bdd, (uint)0x61bdbddc, (uint)0x0d8b8b86, (uint)0x0f8a8a85,
	(uint)0xe0707090, (uint)0x7c3e3e42, (uint)0x71b5b5c4, (uint)0xcc6666aa,
	(uint)0x904848d8, (uint)0x06030305, (uint)0xf7f6f601, (uint)0x1c0e0e12,
	(uint)0xc26161a3, (uint)0x6a35355f, (uint)0xae5757f9, (uint)0x69b9b9d0,
	(uint)0x17868691, (uint)0x99c1c158, (uint)0x3a1d1d27, (uint)0x279e9eb9,
	(uint)0xd9e1e138, (uint)0xebf8f813, (uint)0x2b9898b3, (uint)0x22111133,
	(uint)0xd26969bb, (uint)0xa9d9d970, (uint)0x078e8e89, (uint)0x339494a7,
	(uint)0x2d9b9bb6, (uint)0x3c1e1e22, (uint)0x15878792, (uint)0xc9e9e920,
	(uint)0x87cece49, (uint)0xaa5555ff, (uint)0x50282878, (uint)0xa5dfdf7a,
	(uint)0x038c8c8f, (uint)0x59a1a1f8, (uint)0x09898980, (uint)0x1a0d0d17,
	(uint)0x65bfbfda, (uint)0xd7e6e631, (uint)0x844242c6, (uint)0xd06868b8,
	(uint)0x824141c3, (uint)0x299999b0, (uint)0x5a2d2d77, (uint)0x1e0f0f11,
	(uint)0x7bb0b0cb, (uint)0xa85454fc, (uint)0x6dbbbbd6, (uint)0x2c16163a,
};
static const u32 Te1[256] = {
	(uint)0xa5c66363, (uint)0x84f87c7c, (uint)0x99ee7777, (uint)0x8df67b7b,
	(uint)0x0dfff2f2, (uint)0xbdd66b6b, (uint)0xb1de6f6f, (uint)0x5491c5c5,
	(uint)0x50603030, (uint)0x03020101, (uint)0xa9ce6767, (uint)0x7d562b2b,
	(uint)0x19e7fefe, (uint)0x62b5d7d7, (uint)0xe64dabab, (uint)0x9aec7676,
	(uint)0x458fcaca, (uint)0x9d1f8282, (uint)0x4089c9c9, (uint)0x87fa7d7d,
	(uint)0x15effafa, (uint)0xebb25959, (uint)0xc98e4747, (uint)0x0bfbf0f0,
	(uint)0xec41adad, (uint)0x67b3d4d4, (uint)0xfd5fa2a2, (uint)0xea45afaf,
	(uint)0xbf239c9c, (uint)0xf753a4a4, (uint)0x96e47272, (uint)0x5b9bc0c0,
	(uint)0xc275b7b7, (uint)0x1ce1fdfd, (uint)0xae3d9393, (uint)0x6a4c2626,
	(uint)0x5a6c3636, (uint)0x417e3f3f, (uint)0x02f5f7f7, (uint)0x4f83cccc,
	(uint)0x5c683434, (uint)0xf451a5a5, (uint)0x34d1e5e5, (uint)0x08f9f1f1,
	(uint)0x93e27171, (uint)0x73abd8d8, (uint)0x53623131, (uint)0x3f2a1515,
	(uint)0x0c080404, (uint)0x5295c7c7, (uint)0x65462323, (uint)0x5e9dc3c3,
	(uint)0x28301818, (uint)0xa1379696, (uint)0x0f0a0505, (uint)0xb52f9a9a,
	(uint)0x090e0707, (uint)0x36241212, (uint)0x9b1b8080, (uint)0x3ddfe2e2,
	(uint)0x26cdebeb, (uint)0x694e2727, (uint)0xcd7fb2b2, (uint)0x9fea7575,
	(uint)0x1b120909, (uint)0x9e1d8383, (uint)0x74582c2c, (uint)0x2e341a1a,
	(uint)0x2d361b1b, (uint)0xb2dc6e6e, (uint)0xeeb45a5a, (uint)0xfb5ba0a0,
	(uint)0xf6a45252, (uint)0x4d763b3b, (uint)0x61b7d6d6, (uint)0xce7db3b3,
	(uint)0x7b522929, (uint)0x3edde3e3, (uint)0x715e2f2f, (uint)0x97138484,
	(uint)0xf5a65353, (uint)0x68b9d1d1, (uint)0x00000000, (uint)0x2cc1eded,
	(uint)0x60402020, (uint)0x1fe3fcfc, (uint)0xc879b1b1, (uint)0xedb65b5b,
	(uint)0xbed46a6a, (uint)0x468dcbcb, (uint)0xd967bebe, (uint)0x4b723939,
	(uint)0xde944a4a, (uint)0xd4984c4c, (uint)0xe8b05858, (uint)0x4a85cfcf,
	(uint)0x6bbbd0d0, (uint)0x2ac5efef, (uint)0xe54faaaa, (uint)0x16edfbfb,
	(uint)0xc5864343, (uint)0xd79a4d4d, (uint)0x55663333, (uint)0x94118585,
	(uint)0xcf8a4545, (uint)0x10e9f9f9, (uint)0x06040202, (uint)0x81fe7f7f,
	(uint)0xf0a05050, (uint)0x44783c3c, (uint)0xba259f9f, (uint)0xe34ba8a8,
	(uint)0xf3a25151, (uint)0xfe5da3a3, (uint)0xc0804040, (uint)0x8a058f8f,
	(uint)0xad3f9292, (uint)0xbc219d9d, (uint)0x48703838, (uint)0x04f1f5f5,
	(uint)0xdf63bcbc, (uint)0xc177b6b6, (uint)0x75afdada, (uint)0x63422121,
	(uint)0x30201010, (uint)0x1ae5ffff, (uint)0x0efdf3f3, (uint)0x6dbfd2d2,
	(uint)0x4c81cdcd, (uint)0x14180c0c, (uint)0x35261313, (uint)0x2fc3ecec,
	(uint)0xe1be5f5f, (uint)0xa2359797, (uint)0xcc884444, (uint)0x392e1717,
	(uint)0x5793c4c4, (uint)0xf255a7a7, (uint)0x82fc7e7e, (uint)0x477a3d3d,
	(uint)0xacc86464, (uint)0xe7ba5d5d, (uint)0x2b321919, (uint)0x95e67373,
	(uint)0xa0c06060, (uint)0x98198181, (uint)0xd19e4f4f, (uint)0x7fa3dcdc,
	(uint)0x66442222, (uint)0x7e542a2a, (uint)0xab3b9090, (uint)0x830b8888,
	(uint)0xca8c4646, (uint)0x29c7eeee, (uint)0xd36bb8b8, (uint)0x3c281414,
	(uint)0x79a7dede, (uint)0xe2bc5e5e, (uint)0x1d160b0b, (uint)0x76addbdb,
	(uint)0x3bdbe0e0, (uint)0x56643232, (uint)0x4e743a3a, (uint)0x1e140a0a,
	(uint)0xdb924949, (uint)0x0a0c0606, (uint)0x6c482424, (uint)0xe4b85c5c,
	(uint)0x5d9fc2c2, (uint)0x6ebdd3d3, (uint)0xef43acac, (uint)0xa6c46262,
	(uint)0xa8399191, (uint)0xa4319595, (uint)0x37d3e4e4, (uint)0x8bf27979,
	(uint)0x32d5e7e7, (uint)0x438bc8c8, (uint)0x596e3737, (uint)0xb7da6d6d,
	(uint)0x8c018d8d, (uint)0x64b1d5d5, (uint)0xd29c4e4e, (uint)0xe049a9a9,
	(uint)0xb4d86c6c, (uint)0xfaac5656, (uint)0x07f3f4f4, (uint)0x25cfeaea,
	(uint)0xafca6565, (uint)0x8ef47a7a, (uint)0xe947aeae, (uint)0x18100808,
	(uint)0xd56fbaba, (uint)0x88f07878, (uint)0x6f4a2525, (uint)0x725c2e2e,
	(uint)0x24381c1c, (uint)0xf157a6a6, (uint)0xc773b4b4, (uint)0x5197c6c6,
	(uint)0x23cbe8e8, (uint)0x7ca1dddd, (uint)0x9ce87474, (uint)0x213e1f1f,
	(uint)0xdd964b4b, (uint)0xdc61bdbd, (uint)0x860d8b8b, (uint)0x850f8a8a,
	(uint)0x90e07070, (uint)0x427c3e3e, (uint)0xc471b5b5, (uint)0xaacc6666,
	(uint)0xd8904848, (uint)0x05060303, (uint)0x01f7f6f6, (uint)0x121c0e0e,
	(uint)0xa3c26161, (uint)0x5f6a3535, (uint)0xf9ae5757, (uint)0xd069b9b9,
	(uint)0x91178686, (uint)0x5899c1c1, (uint)0x273a1d1d, (uint)0xb9279e9e,
	(uint)0x38d9e1e1, (uint)0x13ebf8f8, (uint)0xb32b9898, (uint)0x33221111,
	(uint)0xbbd26969, (uint)0x70a9d9d9, (uint)0x89078e8e, (uint)0xa7339494,
	(uint)0xb62d9b9b, (uint)0x223c1e1e, (uint)0x92158787, (uint)0x20c9e9e9,
	(uint)0x4987cece, (uint)0xffaa5555, (uint)0x78502828, (uint)0x7aa5dfdf,
	(uint)0x8f038c8c, (uint)0xf859a1a1, (uint)0x80098989, (uint)0x171a0d0d,
	(uint)0xda65bfbf, (uint)0x31d7e6e6, (uint)0xc6844242, (uint)0xb8d06868,
	(uint)0xc3824141, (uint)0xb0299999, (uint)0x775a2d2d, (uint)0x111e0f0f,
	(uint)0xcb7bb0b0, (uint)0xfca85454, (uint)0xd66dbbbb, (uint)0x3a2c1616,
};
static const u32 Te2[256] = {
	(uint)0x63a5c663, (uint)0x7c84f87c, (uint)0x7799ee77, (uint)0x7b8df67b,
	(uint)0xf20dfff2, (uint)0x6bbdd66b, (uint)0x6fb1de6f, (uint)0xc55491c5,
	(uint)0x30506030, (uint)0x01030201, (uint)0x67a9ce67, (uint)0x2b7d562b,
	(uint)0xfe19e7fe, (uint)0xd762b5d7, (uint)0xabe64dab, (uint)0x769aec76,
	(uint)0xca458fca, (uint)0x829d1f82, (uint)0xc94089c9, (uint)0x7d87fa7d,
	(uint)0xfa15effa, (uint)0x59ebb259, (uint)0x47c98e47, (uint)0xf00bfbf0,
	(uint)0xadec41ad, (uint)0xd467b3d4, (uint)0xa2fd5fa2, (uint)0xafea45af,
	(uint)0x9cbf239c, (uint)0xa4f753a4, (uint)0x7296e472, (uint)0xc05b9bc0,
	(uint)0xb7c275b7, (uint)0xfd1ce1fd, (uint)0x93ae3d93, (uint)0x266a4c26,
	(uint)0x365a6c36, (uint)0x3f417e3f, (uint)0xf702f5f7, (uint)0xcc4f83cc,
	(uint)0x345c6834, (uint)0xa5f451a5, (uint)0xe534d1e5, (uint)0xf108f9f1,
	(uint)0x7193e271, (uint)0xd873abd8, (uint)0x31536231, (uint)0x153f2a15,
	(uint)0x040c0804, (uint)0xc75295c7, (uint)0x23654623, (uint)0xc35e9dc3,
	(uint)0x18283018, (uint)0x96a13796, (uint)0x050f0a05, (uint)0x9ab52f9a,
	(uint)0x07090e07, (uint)0x12362412, (uint)0x809b1b80, (uint)0xe23ddfe2,
	(uint)0xeb26cdeb, (uint)0x27694e27, (uint)0xb2cd7fb2, (uint)0x759fea75,
	(uint)0x091b1209, (uint)0x839e1d83, (uint)0x2c74582c, (uint)0x1a2e341a,
	(uint)0x1b2d361b, (uint)0x6eb2dc6e, (uint)0x5aeeb45a, (uint)0xa0fb5ba0,
	(uint)0x52f6a452, (uint)0x3b4d763b, (uint)0xd661b7d6, (uint)0xb3ce7db3,
	(uint)0x297b5229, (uint)0xe33edde3, (uint)0x2f715e2f, (uint)0x84971384,
	(uint)0x53f5a653, (uint)0xd168b9d1, (uint)0x00000000, (uint)0xed2cc1ed,
	(uint)0x20604020, (uint)0xfc1fe3fc, (uint)0xb1c879b1, (uint)0x5bedb65b,
	(uint)0x6abed46a, (uint)0xcb468dcb, (uint)0xbed967be, (uint)0x394b7239,
	(uint)0x4ade944a, (uint)0x4cd4984c, (uint)0x58e8b058, (uint)0xcf4a85cf,
	(uint)0xd06bbbd0, (uint)0xef2ac5ef, (uint)0xaae54faa, (uint)0xfb16edfb,
	(uint)0x43c58643, (uint)0x4dd79a4d, (uint)0x33556633, (uint)0x85941185,
	(uint)0x45cf8a45, (uint)0xf910e9f9, (uint)0x02060402, (uint)0x7f81fe7f,
	(uint)0x50f0a050, (uint)0x3c44783c, (uint)0x9fba259f, (uint)0xa8e34ba8,
	(uint)0x51f3a251, (uint)0xa3fe5da3, (uint)0x40c08040, (uint)0x8f8a058f,
	(uint)0x92ad3f92, (uint)0x9dbc219d, (uint)0x38487038, (uint)0xf504f1f5,
	(uint)0xbcdf63bc, (uint)0xb6c177b6, (uint)0xda75afda, (uint)0x21634221,
	(uint)0x10302010, (uint)0xff1ae5ff, (uint)0xf30efdf3, (uint)0xd26dbfd2,
	(uint)0xcd4c81cd, (uint)0x0c14180c, (uint)0x13352613, (uint)0xec2fc3ec,
	(uint)0x5fe1be5f, (uint)0x97a23597, (uint)0x44cc8844, (uint)0x17392e17,
	(uint)0xc45793c4, (uint)0xa7f255a7, (uint)0x7e82fc7e, (uint)0x3d477a3d,
	(uint)0x64acc864, (uint)0x5de7ba5d, (uint)0x192b3219, (uint)0x7395e673,
	(uint)0x60a0c060, (uint)0x81981981, (uint)0x4fd19e4f, (uint)0xdc7fa3dc,
	(uint)0x22664422, (uint)0x2a7e542a, (uint)0x90ab3b90, (uint)0x88830b88,
	(uint)0x46ca8c46, (uint)0xee29c7ee, (uint)0xb8d36bb8, (uint)0x143c2814,
	(uint)0xde79a7de, (uint)0x5ee2bc5e, (uint)0x0b1d160b, (uint)0xdb76addb,
	(uint)0xe03bdbe0, (uint)0x32566432, (uint)0x3a4e743a, (uint)0x0a1e140a,
	(uint)0x49db9249, (uint)0x060a0c06, (uint)0x246c4824, (uint)0x5ce4b85c,
	(uint)0xc25d9fc2, (uint)0xd36ebdd3, (uint)0xacef43ac, (uint)0x62a6c462,
	(uint)0x91a83991, (uint)0x95a43195, (uint)0xe437d3e4, (uint)0x798bf279,
	(uint)0xe732d5e7, (uint)0xc8438bc8, (uint)0x37596e37, (uint)0x6db7da6d,
	(uint)0x8d8c018d, (uint)0xd564b1d5, (uint)0x4ed29c4e, (uint)0xa9e049a9,
	(uint)0x6cb4d86c, (uint)0x56faac56, (uint)0xf407f3f4, (uint)0xea25cfea,
	(uint)0x65afca65, (uint)0x7a8ef47a, (uint)0xaee947ae, (uint)0x08181008,
	(uint)0xbad56fba, (uint)0x7888f078, (uint)0x256f4a25, (uint)0x2e725c2e,
	(uint)0x1c24381c, (uint)0xa6f157a6, (uint)0xb4c773b4, (uint)0xc65197c6,
	(uint)0xe823cbe8, (uint)0xdd7ca1dd, (uint)0x749ce874, (uint)0x1f213e1f,
	(uint)0x4bdd964b, (uint)0xbddc61bd, (uint)0x8b860d8b, (uint)0x8a850f8a,
	(uint)0x7090e070, (uint)0x3e427c3e, (uint)0xb5c471b5, (uint)0x66aacc66,
	(uint)0x48d89048, (uint)0x03050603, (uint)0xf601f7f6, (uint)0x0e121c0e,
	(uint)0x61a3c261, (uint)0x355f6a35, (uint)0x57f9ae57, (uint)0xb9d069b9,
	(uint)0x86911786, (uint)0xc15899c1, (uint)0x1d273a1d, (uint)0x9eb9279e,
	(uint)0xe138d9e1, (uint)0xf813ebf8, (uint)0x98b32b98, (uint)0x11332211,
	(uint)0x69bbd269, (uint)0xd970a9d9, (uint)0x8e89078e, (uint)0x94a73394,
	(uint)0x9bb62d9b, (uint)0x1e223c1e, (uint)0x87921587, (uint)0xe920c9e9,
	(uint)0xce4987ce, (uint)0x55ffaa55, (uint)0x28785028, (uint)0xdf7aa5df,
	(uint)0x8c8f038c, (uint)0xa1f859a1, (uint)0x89800989, (uint)0x0d171a0d,
	(uint)0xbfda65bf, (uint)0xe631d7e6, (uint)0x42c68442, (uint)0x68b8d068,
	(uint)0x41c38241, (uint)0x99b02999, (uint)0x2d775a2d, (uint)0x0f111e0f,
	(uint)0xb0cb7bb0, (uint)0x54fca854, (uint)0xbbd66dbb, (uint)0x163a2c16,
};
static const u32 Te3[256] = {

	(uint)0x6363a5c6, (uint)0x7c7c84f8, (uint)0x777799ee, (uint)0x7b7b8df6,
	(uint)0xf2f20dff, (uint)0x6b6bbdd6, (uint)0x6f6fb1de, (uint)0xc5c55491,
	(uint)0x30305060, (uint)0x01010302, (uint)0x6767a9ce, (uint)0x2b2b7d56,
	(uint)0xfefe19e7, (uint)0xd7d762b5, (uint)0xababe64d, (uint)0x76769aec,
	(uint)0xcaca458f, (uint)0x82829d1f, (uint)0xc9c94089, (uint)0x7d7d87fa,
	(uint)0xfafa15ef, (uint)0x5959ebb2, (uint)0x4747c98e, (uint)0xf0f00bfb,
	(uint)0xadadec41, (uint)0xd4d467b3, (uint)0xa2a2fd5f, (uint)0xafafea45,
	(uint)0x9c9cbf23, (uint)0xa4a4f753, (uint)0x727296e4, (uint)0xc0c05b9b,
	(uint)0xb7b7c275, (uint)0xfdfd1ce1, (uint)0x9393ae3d, (uint)0x26266a4c,
	(uint)0x36365a6c, (uint)0x3f3f417e, (uint)0xf7f702f5, (uint)0xcccc4f83,
	(uint)0x34345c68, (uint)0xa5a5f451, (uint)0xe5e534d1, (uint)0xf1f108f9,
	(uint)0x717193e2, (uint)0xd8d873ab, (uint)0x31315362, (uint)0x15153f2a,
	(uint)0x04040c08, (uint)0xc7c75295, (uint)0x23236546, (uint)0xc3c35e9d,
	(uint)0x18182830, (uint)0x9696a137, (uint)0x05050f0a, (uint)0x9a9ab52f,
	(uint)0x0707090e, (uint)0x12123624, (uint)0x80809b1b, (uint)0xe2e23ddf,
	(uint)0xebeb26cd, (uint)0x2727694e, (uint)0xb2b2cd7f, (uint)0x75759fea,
	(uint)0x09091b12, (uint)0x83839e1d, (uint)0x2c2c7458, (uint)0x1a1a2e34,
	(uint)0x1b1b2d36, (uint)0x6e6eb2dc, (uint)0x5a5aeeb4, (uint)0xa0a0fb5b,
	(uint)0x5252f6a4, (uint)0x3b3b4d76, (uint)0xd6d661b7, (uint)0xb3b3ce7d,
	(uint)0x29297b52, (uint)0xe3e33edd, (uint)0x2f2f715e, (uint)0x84849713,
	(uint)0x5353f5a6, (uint)0xd1d168b9, (uint)0x00000000, (uint)0xeded2cc1,
	(uint)0x20206040, (uint)0xfcfc1fe3, (uint)0xb1b1c879, (uint)0x5b5bedb6,
	(uint)0x6a6abed4, (uint)0xcbcb468d, (uint)0xbebed967, (uint)0x39394b72,
	(uint)0x4a4ade94, (uint)0x4c4cd498, (uint)0x5858e8b0, (uint)0xcfcf4a85,
	(uint)0xd0d06bbb, (uint)0xefef2ac5, (uint)0xaaaae54f, (uint)0xfbfb16ed,
	(uint)0x4343c586, (uint)0x4d4dd79a, (uint)0x33335566, (uint)0x85859411,
	(uint)0x4545cf8a, (uint)0xf9f910e9, (uint)0x02020604, (uint)0x7f7f81fe,
	(uint)0x5050f0a0, (uint)0x3c3c4478, (uint)0x9f9fba25, (uint)0xa8a8e34b,
	(uint)0x5151f3a2, (uint)0xa3a3fe5d, (uint)0x4040c080, (uint)0x8f8f8a05,
	(uint)0x9292ad3f, (uint)0x9d9dbc21, (uint)0x38384870, (uint)0xf5f504f1,
	(uint)0xbcbcdf63, (uint)0xb6b6c177, (uint)0xdada75af, (uint)0x21216342,
	(uint)0x10103020, (uint)0xffff1ae5, (uint)0xf3f30efd, (uint)0xd2d26dbf,
	(uint)0xcdcd4c81, (uint)0x0c0c1418, (uint)0x13133526, (uint)0xecec2fc3,
	(uint)0x5f5fe1be, (uint)0x9797a235, (uint)0x4444cc88, (uint)0x1717392e,
	(uint)0xc4c45793, (uint)0xa7a7f255, (uint)0x7e7e82fc, (uint)0x3d3d477a,
	(uint)0x6464acc8, (uint)0x5d5de7ba, (uint)0x19192b32, (uint)0x737395e6,
	(uint)0x6060a0c0, (uint)0x81819819, (uint)0x4f4fd19e, (uint)0xdcdc7fa3,
	(uint)0x22226644, (uint)0x2a2a7e54, (uint)0x9090ab3b, (uint)0x8888830b,
	(uint)0x4646ca8c, (uint)0xeeee29c7, (uint)0xb8b8d36b, (uint)0x14143c28,
	(uint)0xdede79a7, (uint)0x5e5ee2bc, (uint)0x0b0b1d16, (uint)0xdbdb76ad,
	(uint)0xe0e03bdb, (uint)0x32325664, (uint)0x3a3a4e74, (uint)0x0a0a1e14,
	(uint)0x4949db92, (uint)0x06060a0c, (uint)0x24246c48, (uint)0x5c5ce4b8,
	(uint)0xc2c25d9f, (uint)0xd3d36ebd, (uint)0xacacef43, (uint)0x6262a6c4,
	(uint)0x9191a839, (uint)0x9595a431, (uint)0xe4e437d3, (uint)0x79798bf2,
	(uint)0xe7e732d5, (uint)0xc8c8438b, (uint)0x3737596e, (uint)0x6d6db7da,
	(uint)0x8d8d8c01, (uint)0xd5d564b1, (uint)0x4e4ed29c, (uint)0xa9a9e049,
	(uint)0x6c6cb4d8, (uint)0x5656faac, (uint)0xf4f407f3, (uint)0xeaea25cf,
	(uint)0x6565afca, (uint)0x7a7a8ef4, (uint)0xaeaee947, (uint)0x08081810,
	(uint)0xbabad56f, (uint)0x787888f0, (uint)0x25256f4a, (uint)0x2e2e725c,
	(uint)0x1c1c2438, (uint)0xa6a6f157, (uint)0xb4b4c773, (uint)0xc6c65197,
	(uint)0xe8e823cb, (uint)0xdddd7ca1, (uint)0x74749ce8, (uint)0x1f1f213e,
	(uint)0x4b4bdd96, (uint)0xbdbddc61, (uint)0x8b8b860d, (uint)0x8a8a850f,
	(uint)0x707090e0, (uint)0x3e3e427c, (uint)0xb5b5c471, (uint)0x6666aacc,
	(uint)0x4848d890, (uint)0x03030506, (uint)0xf6f601f7, (uint)0x0e0e121c,
	(uint)0x6161a3c2, (uint)0x35355f6a, (uint)0x5757f9ae, (uint)0xb9b9d069,
	(uint)0x86869117, (uint)0xc1c15899, (uint)0x1d1d273a, (uint)0x9e9eb927,
	(uint)0xe1e138d9, (uint)0xf8f813eb, (uint)0x9898b32b, (uint)0x11113322,
	(uint)0x6969bbd2, (uint)0xd9d970a9, (uint)0x8e8e8907, (uint)0x9494a733,
	(uint)0x9b9bb62d, (uint)0x1e1e223c, (uint)0x87879215, (uint)0xe9e920c9,
	(uint)0xcece4987, (uint)0x5555ffaa, (uint)0x28287850, (uint)0xdfdf7aa5,
	(uint)0x8c8c8f03, (uint)0xa1a1f859, (uint)0x89898009, (uint)0x0d0d171a,
	(uint)0xbfbfda65, (uint)0xe6e631d7, (uint)0x4242c684, (uint)0x6868b8d0,
	(uint)0x4141c382, (uint)0x9999b029, (uint)0x2d2d775a, (uint)0x0f0f111e,
	(uint)0xb0b0cb7b, (uint)0x5454fca8, (uint)0xbbbbd66d, (uint)0x16163a2c,
};
static const u32 Te4[256] = {
	(uint)0x63636363, (uint)0x7c7c7c7c, (uint)0x77777777, (uint)0x7b7b7b7b,
	(uint)0xf2f2f2f2, (uint)0x6b6b6b6b, (uint)0x6f6f6f6f, (uint)0xc5c5c5c5,
	(uint)0x30303030, (uint)0x01010101, (uint)0x67676767, (uint)0x2b2b2b2b,
	(uint)0xfefefefe, (uint)0xd7d7d7d7, (uint)0xabababab, (uint)0x76767676,
	(uint)0xcacacaca, (uint)0x82828282, (uint)0xc9c9c9c9, (uint)0x7d7d7d7d,
	(uint)0xfafafafa, (uint)0x59595959, (uint)0x47474747, (uint)0xf0f0f0f0,
	(uint)0xadadadad, (uint)0xd4d4d4d4, (uint)0xa2a2a2a2, (uint)0xafafafaf,
	(uint)0x9c9c9c9c, (uint)0xa4a4a4a4, (uint)0x72727272, (uint)0xc0c0c0c0,
	(uint)0xb7b7b7b7, (uint)0xfdfdfdfd, (uint)0x93939393, (uint)0x26262626,
	(uint)0x36363636, (uint)0x3f3f3f3f, (uint)0xf7f7f7f7, (uint)0xcccccccc,
	(uint)0x34343434, (uint)0xa5a5a5a5, (uint)0xe5e5e5e5, (uint)0xf1f1f1f1,
	(uint)0x71717171, (uint)0xd8d8d8d8, (uint)0x31313131, (uint)0x15151515,
	(uint)0x04040404, (uint)0xc7c7c7c7, (uint)0x23232323, (uint)0xc3c3c3c3,
	(uint)0x18181818, (uint)0x96969696, (uint)0x05050505, (uint)0x9a9a9a9a,
	(uint)0x07070707, (uint)0x12121212, (uint)0x80808080, (uint)0xe2e2e2e2,
	(uint)0xebebebeb, (uint)0x27272727, (uint)0xb2b2b2b2, (uint)0x75757575,
	(uint)0x09090909, (uint)0x83838383, (uint)0x2c2c2c2c, (uint)0x1a1a1a1a,
	(uint)0x1b1b1b1b, (uint)0x6e6e6e6e, (uint)0x5a5a5a5a, (uint)0xa0a0a0a0,
	(uint)0x52525252, (uint)0x3b3b3b3b, (uint)0xd6d6d6d6, (uint)0xb3b3b3b3,
	(uint)0x29292929, (uint)0xe3e3e3e3, (uint)0x2f2f2f2f, (uint)0x84848484,
	(uint)0x53535353, (uint)0xd1d1d1d1, (uint)0x00000000, (uint)0xedededed,
	(uint)0x20202020, (uint)0xfcfcfcfc, (uint)0xb1b1b1b1, (uint)0x5b5b5b5b,
	(uint)0x6a6a6a6a, (uint)0xcbcbcbcb, (uint)0xbebebebe, (uint)0x39393939,
	(uint)0x4a4a4a4a, (uint)0x4c4c4c4c, (uint)0x58585858, (uint)0xcfcfcfcf,
	(uint)0xd0d0d0d0, (uint)0xefefefef, (uint)0xaaaaaaaa, (uint)0xfbfbfbfb,
	(uint)0x43434343, (uint)0x4d4d4d4d, (uint)0x33333333, (uint)0x85858585,
	(uint)0x45454545, (uint)0xf9f9f9f9, (uint)0x02020202, (uint)0x7f7f7f7f,
	(uint)0x50505050, (uint)0x3c3c3c3c, (uint)0x9f9f9f9f, (uint)0xa8a8a8a8,
	(uint)0x51515151, (uint)0xa3a3a3a3, (uint)0x40404040, (uint)0x8f8f8f8f,
	(uint)0x92929292, (uint)0x9d9d9d9d, (uint)0x38383838, (uint)0xf5f5f5f5,
	(uint)0xbcbcbcbc, (uint)0xb6b6b6b6, (uint)0xdadadada, (uint)0x21212121,
	(uint)0x10101010, (uint)0xffffffff, (uint)0xf3f3f3f3, (uint)0xd2d2d2d2,
	(uint)0xcdcdcdcd, (uint)0x0c0c0c0c, (uint)0x13131313, (uint)0xecececec,
	(uint)0x5f5f5f5f, (uint)0x97979797, (uint)0x44444444, (uint)0x17171717,
	(uint)0xc4c4c4c4, (uint)0xa7a7a7a7, (uint)0x7e7e7e7e, (uint)0x3d3d3d3d,
	(uint)0x64646464, (uint)0x5d5d5d5d, (uint)0x19191919, (uint)0x73737373,
	(uint)0x60606060, (uint)0x81818181, (uint)0x4f4f4f4f, (uint)0xdcdcdcdc,
	(uint)0x22222222, (uint)0x2a2a2a2a, (uint)0x90909090, (uint)0x88888888,
	(uint)0x46464646, (uint)0xeeeeeeee, (uint)0xb8b8b8b8, (uint)0x14141414,
	(uint)0xdededede, (uint)0x5e5e5e5e, (uint)0x0b0b0b0b, (uint)0xdbdbdbdb,
	(uint)0xe0e0e0e0, (uint)0x32323232, (uint)0x3a3a3a3a, (uint)0x0a0a0a0a,
	(uint)0x49494949, (uint)0x06060606, (uint)0x24242424, (uint)0x5c5c5c5c,
	(uint)0xc2c2c2c2, (uint)0xd3d3d3d3, (uint)0xacacacac, (uint)0x62626262,
	(uint)0x91919191, (uint)0x95959595, (uint)0xe4e4e4e4, (uint)0x79797979,
	(uint)0xe7e7e7e7, (uint)0xc8c8c8c8, (uint)0x37373737, (uint)0x6d6d6d6d,
	(uint)0x8d8d8d8d, (uint)0xd5d5d5d5, (uint)0x4e4e4e4e, (uint)0xa9a9a9a9,
	(uint)0x6c6c6c6c, (uint)0x56565656, (uint)0xf4f4f4f4, (uint)0xeaeaeaea,
	(uint)0x65656565, (uint)0x7a7a7a7a, (uint)0xaeaeaeae, (uint)0x08080808,
	(uint)0xbabababa, (uint)0x78787878, (uint)0x25252525, (uint)0x2e2e2e2e,
	(uint)0x1c1c1c1c, (uint)0xa6a6a6a6, (uint)0xb4b4b4b4, (uint)0xc6c6c6c6,
	(uint)0xe8e8e8e8, (uint)0xdddddddd, (uint)0x74747474, (uint)0x1f1f1f1f,
	(uint)0x4b4b4b4b, (uint)0xbdbdbdbd, (uint)0x8b8b8b8b, (uint)0x8a8a8a8a,
	(uint)0x70707070, (uint)0x3e3e3e3e, (uint)0xb5b5b5b5, (uint)0x66666666,
	(uint)0x48484848, (uint)0x03030303, (uint)0xf6f6f6f6, (uint)0x0e0e0e0e,
	(uint)0x61616161, (uint)0x35353535, (uint)0x57575757, (uint)0xb9b9b9b9,
	(uint)0x86868686, (uint)0xc1c1c1c1, (uint)0x1d1d1d1d, (uint)0x9e9e9e9e,
	(uint)0xe1e1e1e1, (uint)0xf8f8f8f8, (uint)0x98989898, (uint)0x11111111,
	(uint)0x69696969, (uint)0xd9d9d9d9, (uint)0x8e8e8e8e, (uint)0x94949494,
	(uint)0x9b9b9b9b, (uint)0x1e1e1e1e, (uint)0x87878787, (uint)0xe9e9e9e9,
	(uint)0xcececece, (uint)0x55555555, (uint)0x28282828, (uint)0xdfdfdfdf,
	(uint)0x8c8c8c8c, (uint)0xa1a1a1a1, (uint)0x89898989, (uint)0x0d0d0d0d,
	(uint)0xbfbfbfbf, (uint)0xe6e6e6e6, (uint)0x42424242, (uint)0x68686868,
	(uint)0x41414141, (uint)0x99999999, (uint)0x2d2d2d2d, (uint)0x0f0f0f0f,
	(uint)0xb0b0b0b0, (uint)0x54545454, (uint)0xbbbbbbbb, (uint)0x16161616,
};
static const u32 Td0[256] = {
	(uint)0x51f4a750, (uint)0x7e416553, (uint)0x1a17a4c3, (uint)0x3a275e96,
	(uint)0x3bab6bcb, (uint)0x1f9d45f1, (uint)0xacfa58ab, (uint)0x4be30393,
	(uint)0x2030fa55, (uint)0xad766df6, (uint)0x88cc7691, (uint)0xf5024c25,
	(uint)0x4fe5d7fc, (uint)0xc52acbd7, (uint)0x26354480, (uint)0xb562a38f,
	(uint)0xdeb15a49, (uint)0x25ba1b67, (uint)0x45ea0e98, (uint)0x5dfec0e1,
	(uint)0xc32f7502, (uint)0x814cf012, (uint)0x8d4697a3, (uint)0x6bd3f9c6,
	(uint)0x038f5fe7, (uint)0x15929c95, (uint)0xbf6d7aeb, (uint)0x955259da,
	(uint)0xd4be832d, (uint)0x587421d3, (uint)0x49e06929, (uint)0x8ec9c844,
	(uint)0x75c2896a, (uint)0xf48e7978, (uint)0x99583e6b, (uint)0x27b971dd,
	(uint)0xbee14fb6, (uint)0xf088ad17, (uint)0xc920ac66, (uint)0x7dce3ab4,
	(uint)0x63df4a18, (uint)0xe51a3182, (uint)0x97513360, (uint)0x62537f45,
	(uint)0xb16477e0, (uint)0xbb6bae84, (uint)0xfe81a01c, (uint)0xf9082b94,
	(uint)0x70486858, (uint)0x8f45fd19, (uint)0x94de6c87, (uint)0x527bf8b7,
	(uint)0xab73d323, (uint)0x724b02e2, (uint)0xe31f8f57, (uint)0x6655ab2a,
	(uint)0xb2eb2807, (uint)0x2fb5c203, (uint)0x86c57b9a, (uint)0xd33708a5,
	(uint)0x302887f2, (uint)0x23bfa5b2, (uint)0x02036aba, (uint)0xed16825c,
	(uint)0x8acf1c2b, (uint)0xa779b492, (uint)0xf307f2f0, (uint)0x4e69e2a1,
	(uint)0x65daf4cd, (uint)0x0605bed5, (uint)0xd134621f, (uint)0xc4a6fe8a,
	(uint)0x342e539d, (uint)0xa2f355a0, (uint)0x058ae132, (uint)0xa4f6eb75,
	(uint)0x0b83ec39, (uint)0x4060efaa, (uint)0x5e719f06, (uint)0xbd6e1051,
	(uint)0x3e218af9, (uint)0x96dd063d, (uint)0xdd3e05ae, (uint)0x4de6bd46,
	(uint)0x91548db5, (uint)0x71c45d05, (uint)0x0406d46f, (uint)0x605015ff,
	(uint)0x1998fb24, (uint)0xd6bde997, (uint)0x894043cc, (uint)0x67d99e77,
	(uint)0xb0e842bd, (uint)0x07898b88, (uint)0xe7195b38, (uint)0x79c8eedb,
	(uint)0xa17c0a47, (uint)0x7c420fe9, (uint)0xf8841ec9, (uint)0x00000000,
	(uint)0x09808683, (uint)0x322bed48, (uint)0x1e1170ac, (uint)0x6c5a724e,
	(uint)0xfd0efffb, (uint)0x0f853856, (uint)0x3daed51e, (uint)0x362d3927,
	(uint)0x0a0fd964, (uint)0x685ca621, (uint)0x9b5b54d1, (uint)0x24362e3a,
	(uint)0x0c0a67b1, (uint)0x9357e70f, (uint)0xb4ee96d2, (uint)0x1b9b919e,
	(uint)0x80c0c54f, (uint)0x61dc20a2, (uint)0x5a774b69, (uint)0x1c121a16,
	(uint)0xe293ba0a, (uint)0xc0a02ae5, (uint)0x3c22e043, (uint)0x121b171d,
	(uint)0x0e090d0b, (uint)0xf28bc7ad, (uint)0x2db6a8b9, (uint)0x141ea9c8,
	(uint)0x57f11985, (uint)0xaf75074c, (uint)0xee99ddbb, (uint)0xa37f60fd,
	(uint)0xf701269f, (uint)0x5c72f5bc, (uint)0x44663bc5, (uint)0x5bfb7e34,
	(uint)0x8b432976, (uint)0xcb23c6dc, (uint)0xb6edfc68, (uint)0xb8e4f163,
	(uint)0xd731dcca, (uint)0x42638510, (uint)0x13972240, (uint)0x84c61120,
	(uint)0x854a247d, (uint)0xd2bb3df8, (uint)0xaef93211, (uint)0xc729a16d,
	(uint)0x1d9e2f4b, (uint)0xdcb230f3, (uint)0x0d8652ec, (uint)0x77c1e3d0,
	(uint)0x2bb3166c, (uint)0xa970b999, (uint)0x119448fa, (uint)0x47e96422,
	(uint)0xa8fc8cc4, (uint)0xa0f03f1a, (uint)0x567d2cd8, (uint)0x223390ef,
	(uint)0x87494ec7, (uint)0xd938d1c1, (uint)0x8ccaa2fe, (uint)0x98d40b36,
	(uint)0xa6f581cf, (uint)0xa57ade28, (uint)0xdab78e26, (uint)0x3fadbfa4,
	(uint)0x2c3a9de4, (uint)0x5078920d, (uint)0x6a5fcc9b, (uint)0x547e4662,
	(uint)0xf68d13c2, (uint)0x90d8b8e8, (uint)0x2e39f75e, (uint)0x82c3aff5,
	(uint)0x9f5d80be, (uint)0x69d0937c, (uint)0x6fd52da9, (uint)0xcf2512b3,
	(uint)0xc8ac993b, (uint)0x10187da7, (uint)0xe89c636e, (uint)0xdb3bbb7b,
	(uint)0xcd267809, (uint)0x6e5918f4, (uint)0xec9ab701, (uint)0x834f9aa8,
	(uint)0xe6956e65, (uint)0xaaffe67e, (uint)0x21bccf08, (uint)0xef15e8e6,
	(uint)0xbae79bd9, (uint)0x4a6f36ce, (uint)0xea9f09d4, (uint)0x29b07cd6,
	(uint)0x31a4b2af, (uint)0x2a3f2331, (uint)0xc6a59430, (uint)0x35a266c0,
	(uint)0x744ebc37, (uint)0xfc82caa6, (uint)0xe090d0b0, (uint)0x33a7d815,
	(uint)0xf104984a, (uint)0x41ecdaf7, (uint)0x7fcd500e, (uint)0x1791f62f,
	(uint)0x764dd68d, (uint)0x43efb04d, (uint)0xccaa4d54, (uint)0xe49604df,
	(uint)0x9ed1b5e3, (uint)0x4c6a881b, (uint)0xc12c1fb8, (uint)0x4665517f,
	(uint)0x9d5eea04, (uint)0x018c355d, (uint)0xfa877473, (uint)0xfb0b412e,
	(uint)0xb3671d5a, (uint)0x92dbd252, (uint)0xe9105633, (uint)0x6dd64713,
	(uint)0x9ad7618c, (uint)0x37a10c7a, (uint)0x59f8148e, (uint)0xeb133c89,
	(uint)0xcea927ee, (uint)0xb761c935, (uint)0xe11ce5ed, (uint)0x7a47b13c,
	(uint)0x9cd2df59, (uint)0x55f2733f, (uint)0x1814ce79, (uint)0x73c737bf,
	(uint)0x53f7cdea, (uint)0x5ffdaa5b, (uint)0xdf3d6f14, (uint)0x7844db86,
	(uint)0xcaaff381, (uint)0xb968c43e, (uint)0x3824342c, (uint)0xc2a3405f,
	(uint)0x161dc372, (uint)0xbce2250c, (uint)0x283c498b, (uint)0xff0d9541,
	(uint)0x39a80171, (uint)0x080cb3de, (uint)0xd8b4e49c, (uint)0x6456c190,
	(uint)0x7bcb8461, (uint)0xd532b670, (uint)0x486c5c74, (uint)0xd0b85742,
};
static const u32 Td1[256] = {
	(uint)0x5051f4a7, (uint)0x537e4165, (uint)0xc31a17a4, (uint)0x963a275e,
	(uint)0xcb3bab6b, (uint)0xf11f9d45, (uint)0xabacfa58, (uint)0x934be303,
	(uint)0x552030fa, (uint)0xf6ad766d, (uint)0x9188cc76, (uint)0x25f5024c,
	(uint)0xfc4fe5d7, (uint)0xd7c52acb, (uint)0x80263544, (uint)0x8fb562a3,
	(uint)0x49deb15a, (uint)0x6725ba1b, (uint)0x9845ea0e, (uint)0xe15dfec0,
	(uint)0x02c32f75, (uint)0x12814cf0, (uint)0xa38d4697, (uint)0xc66bd3f9,
	(uint)0xe7038f5f, (uint)0x9515929c, (uint)0xebbf6d7a, (uint)0xda955259,
	(uint)0x2dd4be83, (uint)0xd3587421, (uint)0x2949e069, (uint)0x448ec9c8,
	(uint)0x6a75c289, (uint)0x78f48e79, (uint)0x6b99583e, (uint)0xdd27b971,
	(uint)0xb6bee14f, (uint)0x17f088ad, (uint)0x66c920ac, (uint)0xb47dce3a,
	(uint)0x1863df4a, (uint)0x82e51a31, (uint)0x60975133, (uint)0x4562537f,
	(uint)0xe0b16477, (uint)0x84bb6bae, (uint)0x1cfe81a0, (uint)0x94f9082b,
	(uint)0x58704868, (uint)0x198f45fd, (uint)0x8794de6c, (uint)0xb7527bf8,
	(uint)0x23ab73d3, (uint)0xe2724b02, (uint)0x57e31f8f, (uint)0x2a6655ab,
	(uint)0x07b2eb28, (uint)0x032fb5c2, (uint)0x9a86c57b, (uint)0xa5d33708,
	(uint)0xf2302887, (uint)0xb223bfa5, (uint)0xba02036a, (uint)0x5ced1682,
	(uint)0x2b8acf1c, (uint)0x92a779b4, (uint)0xf0f307f2, (uint)0xa14e69e2,
	(uint)0xcd65daf4, (uint)0xd50605be, (uint)0x1fd13462, (uint)0x8ac4a6fe,
	(uint)0x9d342e53, (uint)0xa0a2f355, (uint)0x32058ae1, (uint)0x75a4f6eb,
	(uint)0x390b83ec, (uint)0xaa4060ef, (uint)0x065e719f, (uint)0x51bd6e10,
	(uint)0xf93e218a, (uint)0x3d96dd06, (uint)0xaedd3e05, (uint)0x464de6bd,
	(uint)0xb591548d, (uint)0x0571c45d, (uint)0x6f0406d4, (uint)0xff605015,
	(uint)0x241998fb, (uint)0x97d6bde9, (uint)0xcc894043, (uint)0x7767d99e,
	(uint)0xbdb0e842, (uint)0x8807898b, (uint)0x38e7195b, (uint)0xdb79c8ee,
	(uint)0x47a17c0a, (uint)0xe97c420f, (uint)0xc9f8841e, (uint)0x00000000,
	(uint)0x83098086, (uint)0x48322bed, (uint)0xac1e1170, (uint)0x4e6c5a72,
	(uint)0xfbfd0eff, (uint)0x560f8538, (uint)0x1e3daed5, (uint)0x27362d39,
	(uint)0x640a0fd9, (uint)0x21685ca6, (uint)0xd19b5b54, (uint)0x3a24362e,
	(uint)0xb10c0a67, (uint)0x0f9357e7, (uint)0xd2b4ee96, (uint)0x9e1b9b91,
	(uint)0x4f80c0c5, (uint)0xa261dc20, (uint)0x695a774b, (uint)0x161c121a,
	(uint)0x0ae293ba, (uint)0xe5c0a02a, (uint)0x433c22e0, (uint)0x1d121b17,
	(uint)0x0b0e090d, (uint)0xadf28bc7, (uint)0xb92db6a8, (uint)0xc8141ea9,
	(uint)0x8557f119, (uint)0x4caf7507, (uint)0xbbee99dd, (uint)0xfda37f60,
	(uint)0x9ff70126, (uint)0xbc5c72f5, (uint)0xc544663b, (uint)0x345bfb7e,
	(uint)0x768b4329, (uint)0xdccb23c6, (uint)0x68b6edfc, (uint)0x63b8e4f1,
	(uint)0xcad731dc, (uint)0x10426385, (uint)0x40139722, (uint)0x2084c611,
	(uint)0x7d854a24, (uint)0xf8d2bb3d, (uint)0x11aef932, (uint)0x6dc729a1,
	(uint)0x4b1d9e2f, (uint)0xf3dcb230, (uint)0xec0d8652, (uint)0xd077c1e3,
	(uint)0x6c2bb316, (uint)0x99a970b9, (uint)0xfa119448, (uint)0x2247e964,
	(uint)0xc4a8fc8c, (uint)0x1aa0f03f, (uint)0xd8567d2c, (uint)0xef223390,
	(uint)0xc787494e, (uint)0xc1d938d1, (uint)0xfe8ccaa2, (uint)0x3698d40b,
	(uint)0xcfa6f581, (uint)0x28a57ade, (uint)0x26dab78e, (uint)0xa43fadbf,
	(uint)0xe42c3a9d, (uint)0x0d507892, (uint)0x9b6a5fcc, (uint)0x62547e46,
	(uint)0xc2f68d13, (uint)0xe890d8b8, (uint)0x5e2e39f7, (uint)0xf582c3af,
	(uint)0xbe9f5d80, (uint)0x7c69d093, (uint)0xa96fd52d, (uint)0xb3cf2512,
	(uint)0x3bc8ac99, (uint)0xa710187d, (uint)0x6ee89c63, (uint)0x7bdb3bbb,
	(uint)0x09cd2678, (uint)0xf46e5918, (uint)0x01ec9ab7, (uint)0xa8834f9a,
	(uint)0x65e6956e, (uint)0x7eaaffe6, (uint)0x0821bccf, (uint)0xe6ef15e8,
	(uint)0xd9bae79b, (uint)0xce4a6f36, (uint)0xd4ea9f09, (uint)0xd629b07c,
	(uint)0xaf31a4b2, (uint)0x312a3f23, (uint)0x30c6a594, (uint)0xc035a266,
	(uint)0x37744ebc, (uint)0xa6fc82ca, (uint)0xb0e090d0, (uint)0x1533a7d8,
	(uint)0x4af10498, (uint)0xf741ecda, (uint)0x0e7fcd50, (uint)0x2f1791f6,
	(uint)0x8d764dd6, (uint)0x4d43efb0, (uint)0x54ccaa4d, (uint)0xdfe49604,
	(uint)0xe39ed1b5, (uint)0x1b4c6a88, (uint)0xb8c12c1f, (uint)0x7f466551,
	(uint)0x049d5eea, (uint)0x5d018c35, (uint)0x73fa8774, (uint)0x2efb0b41,
	(uint)0x5ab3671d, (uint)0x5292dbd2, (uint)0x33e91056, (uint)0x136dd647,
	(uint)0x8c9ad761, (uint)0x7a37a10c, (uint)0x8e59f814, (uint)0x89eb133c,
	(uint)0xeecea927, (uint)0x35b761c9, (uint)0xede11ce5, (uint)0x3c7a47b1,
	(uint)0x599cd2df, (uint)0x3f55f273, (uint)0x791814ce, (uint)0xbf73c737,
	(uint)0xea53f7cd, (uint)0x5b5ffdaa, (uint)0x14df3d6f, (uint)0x867844db,
	(uint)0x81caaff3, (uint)0x3eb968c4, (uint)0x2c382434, (uint)0x5fc2a340,
	(uint)0x72161dc3, (uint)0x0cbce225, (uint)0x8b283c49, (uint)0x41ff0d95,
	(uint)0x7139a801, (uint)0xde080cb3, (uint)0x9cd8b4e4, (uint)0x906456c1,
	(uint)0x617bcb84, (uint)0x70d532b6, (uint)0x74486c5c, (uint)0x42d0b857,
};
static const u32 Td2[256] = {
	(uint)0xa75051f4, (uint)0x65537e41, (uint)0xa4c31a17, (uint)0x5e963a27,
	(uint)0x6bcb3bab, (uint)0x45f11f9d, (uint)0x58abacfa, (uint)0x03934be3,
	(uint)0xfa552030, (uint)0x6df6ad76, (uint)0x769188cc, (uint)0x4c25f502,
	(uint)0xd7fc4fe5, (uint)0xcbd7c52a, (uint)0x44802635, (uint)0xa38fb562,
	(uint)0x5a49deb1, (uint)0x1b6725ba, (uint)0x0e9845ea, (uint)0xc0e15dfe,
	(uint)0x7502c32f, (uint)0xf012814c, (uint)0x97a38d46, (uint)0xf9c66bd3,
	(uint)0x5fe7038f, (uint)0x9c951592, (uint)0x7aebbf6d, (uint)0x59da9552,
	(uint)0x832dd4be, (uint)0x21d35874, (uint)0x692949e0, (uint)0xc8448ec9,
	(uint)0x896a75c2, (uint)0x7978f48e, (uint)0x3e6b9958, (uint)0x71dd27b9,
	(uint)0x4fb6bee1, (uint)0xad17f088, (uint)0xac66c920, (uint)0x3ab47dce,
	(uint)0x4a1863df, (uint)0x3182e51a, (uint)0x33609751, (uint)0x7f456253,
	(uint)0x77e0b164, (uint)0xae84bb6b, (uint)0xa01cfe81, (uint)0x2b94f908,
	(uint)0x68587048, (uint)0xfd198f45, (uint)0x6c8794de, (uint)0xf8b7527b,
	(uint)0xd323ab73, (uint)0x02e2724b, (uint)0x8f57e31f, (uint)0xab2a6655,
	(uint)0x2807b2eb, (uint)0xc2032fb5, (uint)0x7b9a86c5, (uint)0x08a5d337,
	(uint)0x87f23028, (uint)0xa5b223bf, (uint)0x6aba0203, (uint)0x825ced16,
	(uint)0x1c2b8acf, (uint)0xb492a779, (uint)0xf2f0f307, (uint)0xe2a14e69,
	(uint)0xf4cd65da, (uint)0xbed50605, (uint)0x621fd134, (uint)0xfe8ac4a6,
	(uint)0x539d342e, (uint)0x55a0a2f3, (uint)0xe132058a, (uint)0xeb75a4f6,
	(uint)0xec390b83, (uint)0xefaa4060, (uint)0x9f065e71, (uint)0x1051bd6e,

	(uint)0x8af93e21, (uint)0x063d96dd, (uint)0x05aedd3e, (uint)0xbd464de6,
	(uint)0x8db59154, (uint)0x5d0571c4, (uint)0xd46f0406, (uint)0x15ff6050,
	(uint)0xfb241998, (uint)0xe997d6bd, (uint)0x43cc8940, (uint)0x9e7767d9,
	(uint)0x42bdb0e8, (uint)0x8b880789, (uint)0x5b38e719, (uint)0xeedb79c8,
	(uint)0x0a47a17c, (uint)0x0fe97c42, (uint)0x1ec9f884, (uint)0x00000000,
	(uint)0x86830980, (uint)0xed48322b, (uint)0x70ac1e11, (uint)0x724e6c5a,
	(uint)0xfffbfd0e, (uint)0x38560f85, (uint)0xd51e3dae, (uint)0x3927362d,
	(uint)0xd9640a0f, (uint)0xa621685c, (uint)0x54d19b5b, (uint)0x2e3a2436,
	(uint)0x67b10c0a, (uint)0xe70f9357, (uint)0x96d2b4ee, (uint)0x919e1b9b,
	(uint)0xc54f80c0, (uint)0x20a261dc, (uint)0x4b695a77, (uint)0x1a161c12,
	(uint)0xba0ae293, (uint)0x2ae5c0a0, (uint)0xe0433c22, (uint)0x171d121b,
	(uint)0x0d0b0e09, (uint)0xc7adf28b, (uint)0xa8b92db6, (uint)0xa9c8141e,
	(uint)0x198557f1, (uint)0x074caf75, (uint)0xddbbee99, (uint)0x60fda37f,
	(uint)0x269ff701, (uint)0xf5bc5c72, (uint)0x3bc54466, (uint)0x7e345bfb,
	(uint)0x29768b43, (uint)0xc6dccb23, (uint)0xfc68b6ed, (uint)0xf163b8e4,
	(uint)0xdccad731, (uint)0x85104263, (uint)0x22401397, (uint)0x112084c6,
	(uint)0x247d854a, (uint)0x3df8d2bb, (uint)0x3211aef9, (uint)0xa16dc729,
	(uint)0x2f4b1d9e, (uint)0x30f3dcb2, (uint)0x52ec0d86, (uint)0xe3d077c1,
	(uint)0x166c2bb3, (uint)0xb999a970, (uint)0x48fa1194, (uint)0x642247e9,
	(uint)0x8cc4a8fc, (uint)0x3f1aa0f0, (uint)0x2cd8567d, (uint)0x90ef2233,
	(uint)0x4ec78749, (uint)0xd1c1d938, (uint)0xa2fe8cca, (uint)0x0b3698d4,
	(uint)0x81cfa6f5, (uint)0xde28a57a, (uint)0x8e26dab7, (uint)0xbfa43fad,
	(uint)0x9de42c3a, (uint)0x920d5078, (uint)0xcc9b6a5f, (uint)0x4662547e,
	(uint)0x13c2f68d, (uint)0xb8e890d8, (uint)0xf75e2e39, (uint)0xaff582c3,
	(uint)0x80be9f5d, (uint)0x937c69d0, (uint)0x2da96fd5, (uint)0x12b3cf25,
	(uint)0x993bc8ac, (uint)0x7da71018, (uint)0x636ee89c, (uint)0xbb7bdb3b,
	(uint)0x7809cd26, (uint)0x18f46e59, (uint)0xb701ec9a, (uint)0x9aa8834f,
	(uint)0x6e65e695, (uint)0xe67eaaff, (uint)0xcf0821bc, (uint)0xe8e6ef15,
	(uint)0x9bd9bae7, (uint)0x36ce4a6f, (uint)0x09d4ea9f, (uint)0x7cd629b0,
	(uint)0xb2af31a4, (uint)0x23312a3f, (uint)0x9430c6a5, (uint)0x66c035a2,
	(uint)0xbc37744e, (uint)0xcaa6fc82, (uint)0xd0b0e090, (uint)0xd81533a7,
	(uint)0x984af104, (uint)0xdaf741ec, (uint)0x500e7fcd, (uint)0xf62f1791,
	(uint)0xd68d764d, (uint)0xb04d43ef, (uint)0x4d54ccaa, (uint)0x04dfe496,
	(uint)0xb5e39ed1, (uint)0x881b4c6a, (uint)0x1fb8c12c, (uint)0x517f4665,
	(uint)0xea049d5e, (uint)0x355d018c, (uint)0x7473fa87, (uint)0x412efb0b,
	(uint)0x1d5ab367, (uint)0xd25292db, (uint)0x5633e910, (uint)0x47136dd6,
	(uint)0x618c9ad7, (uint)0x0c7a37a1, (uint)0x148e59f8, (uint)0x3c89eb13,
	(uint)0x27eecea9, (uint)0xc935b761, (uint)0xe5ede11c, (uint)0xb13c7a47,
	(uint)0xdf599cd2, (uint)0x733f55f2, (uint)0xce791814, (uint)0x37bf73c7,
	(uint)0xcdea53f7, (uint)0xaa5b5ffd, (uint)0x6f14df3d, (uint)0xdb867844,
	(uint)0xf381caaf, (uint)0xc43eb968, (uint)0x342c3824, (uint)0x405fc2a3,
	(uint)0xc372161d, (uint)0x250cbce2, (uint)0x498b283c, (uint)0x9541ff0d,
	(uint)0x017139a8, (uint)0xb3de080c, (uint)0xe49cd8b4, (uint)0xc1906456,
	(uint)0x84617bcb, (uint)0xb670d532, (uint)0x5c74486c, (uint)0x5742d0b8,
};
static const u32 Td3[256] = {
	(uint)0xf4a75051, (uint)0x4165537e, (uint)0x17a4c31a, (uint)0x275e963a,
	(uint)0xab6bcb3b, (uint)0x9d45f11f, (uint)0xfa58abac, (uint)0xe303934b,
	(uint)0x30fa5520, (uint)0x766df6ad, (uint)0xcc769188, (uint)0x024c25f5,
	(uint)0xe5d7fc4f, (uint)0x2acbd7c5, (uint)0x35448026, (uint)0x62a38fb5,
	(uint)0xb15a49de, (uint)0xba1b6725, (uint)0xea0e9845, (uint)0xfec0e15d,
	(uint)0x2f7502c3, (uint)0x4cf01281, (uint)0x4697a38d, (uint)0xd3f9c66b,
	(uint)0x8f5fe703, (uint)0x929c9515, (uint)0x6d7aebbf, (uint)0x5259da95,
	(uint)0xbe832dd4, (uint)0x7421d358, (uint)0xe0692949, (uint)0xc9c8448e,
	(uint)0xc2896a75, (uint)0x8e7978f4, (uint)0x583e6b99, (uint)0xb971dd27,
	(uint)0xe14fb6be, (uint)0x88ad17f0, (uint)0x20ac66c9, (uint)0xce3ab47d,
	(uint)0xdf4a1863, (uint)0x1a3182e5, (uint)0x51336097, (uint)0x537f4562,
	(uint)0x6477e0b1, (uint)0x6bae84bb, (uint)0x81a01cfe, (uint)0x082b94f9,
	(uint)0x48685870, (uint)0x45fd198f, (uint)0xde6c8794, (uint)0x7bf8b752,
	(uint)0x73d323ab, (uint)0x4b02e272, (uint)0x1f8f57e3, (uint)0x55ab2a66,
	(uint)0xeb2807b2, (uint)0xb5c2032f, (uint)0xc57b9a86, (uint)0x3708a5d3,
	(uint)0x2887f230, (uint)0xbfa5b223, (uint)0x036aba02, (uint)0x16825ced,
	(uint)0xcf1c2b8a, (uint)0x79b492a7, (uint)0x07f2f0f3, (uint)0x69e2a14e,
	(uint)0xdaf4cd65, (uint)0x05bed506, (uint)0x34621fd1, (uint)0xa6fe8ac4,
	(uint)0x2e539d34, (uint)0xf355a0a2, (uint)0x8ae13205, (uint)0xf6eb75a4,
	(uint)0x83ec390b, (uint)0x60efaa40, (uint)0x719f065e, (uint)0x6e1051bd,
	(uint)0x218af93e, (uint)0xdd063d96, (uint)0x3e05aedd, (uint)0xe6bd464d,
	(uint)0x548db591, (uint)0xc45d0571, (uint)0x06d46f04, (uint)0x5015ff60,
	(uint)0x98fb2419, (uint)0xbde997d6, (uint)0x4043cc89, (uint)0xd99e7767,
	(uint)0xe842bdb0, (uint)0x898b8807, (uint)0x195b38e7, (uint)0xc8eedb79,
	(uint)0x7c0a47a1, (uint)0x420fe97c, (uint)0x841ec9f8, (uint)0x00000000,
	(uint)0x80868309, (uint)0x2bed4832, (uint)0x1170ac1e, (uint)0x5a724e6c,
	(uint)0x0efffbfd, (uint)0x8538560f, (uint)0xaed51e3d, (uint)0x2d392736,
	(uint)0x0fd9640a, (uint)0x5ca62168, (uint)0x5b54d19b, (uint)0x362e3a24,
	(uint)0x0a67b10c, (uint)0x57e70f93, (uint)0xee96d2b4, (uint)0x9b919e1b,
	(uint)0xc0c54f80, (uint)0xdc20a261, (uint)0x774b695a, (uint)0x121a161c,
	(uint)0x93ba0ae2, (uint)0xa02ae5c0, (uint)0x22e0433c, (uint)0x1b171d12,
	(uint)0x090d0b0e, (uint)0x8bc7adf2, (uint)0xb6a8b92d, (uint)0x1ea9c814,
	(uint)0xf1198557, (uint)0x75074caf, (uint)0x99ddbbee, (uint)0x7f60fda3,
	(uint)0x01269ff7, (uint)0x72f5bc5c, (uint)0x663bc544, (uint)0xfb7e345b,
	(uint)0x4329768b, (uint)0x23c6dccb, (uint)0xedfc68b6, (uint)0xe4f163b8,
	(uint)0x31dccad7, (uint)0x63851042, (uint)0x97224013, (uint)0xc6112084,
	(uint)0x4a247d85, (uint)0xbb3df8d2, (uint)0xf93211ae, (uint)0x29a16dc7,
	(uint)0x9e2f4b1d, (uint)0xb230f3dc, (uint)0x8652ec0d, (uint)0xc1e3d077,
	(uint)0xb3166c2b, (uint)0x70b999a9, (uint)0x9448fa11, (uint)0xe9642247,
	(uint)0xfc8cc4a8, (uint)0xf03f1aa0, (uint)0x7d2cd856, (uint)0x3390ef22,
	(uint)0x494ec787, (uint)0x38d1c1d9, (uint)0xcaa2fe8c, (uint)0xd40b3698,
	(uint)0xf581cfa6, (uint)0x7ade28a5, (uint)0xb78e26da, (uint)0xadbfa43f,
	(uint)0x3a9de42c, (uint)0x78920d50, (uint)0x5fcc9b6a, (uint)0x7e466254,
	(uint)0x8d13c2f6, (uint)0xd8b8e890, (uint)0x39f75e2e, (uint)0xc3aff582,
	(uint)0x5d80be9f, (uint)0xd0937c69, (uint)0xd52da96f, (uint)0x2512b3cf,
	(uint)0xac993bc8, (uint)0x187da710, (uint)0x9c636ee8, (uint)0x3bbb7bdb,
	(uint)0x267809cd, (uint)0x5918f46e, (uint)0x9ab701ec, (uint)0x4f9aa883,
	(uint)0x956e65e6, (uint)0xffe67eaa, (uint)0xbccf0821, (uint)0x15e8e6ef,
	(uint)0xe79bd9ba, (uint)0x6f36ce4a, (uint)0x9f09d4ea, (uint)0xb07cd629,
	(uint)0xa4b2af31, (uint)0x3f23312a, (uint)0xa59430c6, (uint)0xa266c035,
	(uint)0x4ebc3774, (uint)0x82caa6fc, (uint)0x90d0b0e0, (uint)0xa7d81533,
	(uint)0x04984af1, (uint)0xecdaf741, (uint)0xcd500e7f, (uint)0x91f62f17,
	(uint)0x4dd68d76, (uint)0xefb04d43, (uint)0xaa4d54cc, (uint)0x9604dfe4,
	(uint)0xd1b5e39e, (uint)0x6a881b4c, (uint)0x2c1fb8c1, (uint)0x65517f46,
	(uint)0x5eea049d, (uint)0x8c355d01, (uint)0x877473fa, (uint)0x0b412efb,
	(uint)0x671d5ab3, (uint)0xdbd25292, (uint)0x105633e9, (uint)0xd647136d,
	(uint)0xd7618c9a, (uint)0xa10c7a37, (uint)0xf8148e59, (uint)0x133c89eb,
	(uint)0xa927eece, (uint)0x61c935b7, (uint)0x1ce5ede1, (uint)0x47b13c7a,
	(uint)0xd2df599c, (uint)0xf2733f55, (uint)0x14ce7918, (uint)0xc737bf73,
	(uint)0xf7cdea53, (uint)0xfdaa5b5f, (uint)0x3d6f14df, (uint)0x44db8678,
	(uint)0xaff381ca, (uint)0x68c43eb9, (uint)0x24342c38, (uint)0xa3405fc2,
	(uint)0x1dc37216, (uint)0xe2250cbc, (uint)0x3c498b28, (uint)0x0d9541ff,
	(uint)0xa8017139, (uint)0x0cb3de08, (uint)0xb4e49cd8, (uint)0x56c19064,
	(uint)0xcb84617b, (uint)0x32b670d5, (uint)0x6c5c7448, (uint)0xb85742d0,
};
static const u32 Td4[256] = {
	(uint)0x52525252, (uint)0x09090909, (uint)0x6a6a6a6a, (uint)0xd5d5d5d5,
	(uint)0x30303030, (uint)0x36363636, (uint)0xa5a5a5a5, (uint)0x38383838,
	(uint)0xbfbfbfbf, (uint)0x40404040, (uint)0xa3a3a3a3, (uint)0x9e9e9e9e,
	(uint)0x81818181, (uint)0xf3f3f3f3, (uint)0xd7d7d7d7, (uint)0xfbfbfbfb,
	(uint)0x7c7c7c7c, (uint)0xe3e3e3e3, (uint)0x39393939, (uint)0x82828282,
	(uint)0x9b9b9b9b, (uint)0x2f2f2f2f, (uint)0xffffffff, (uint)0x87878787,
	(uint)0x34343434, (uint)0x8e8e8e8e, (uint)0x43434343, (uint)0x44444444,
	(uint)0xc4c4c4c4, (uint)0xdededede, (uint)0xe9e9e9e9, (uint)0xcbcbcbcb,
	(uint)0x54545454, (uint)0x7b7b7b7b, (uint)0x94949494, (uint)0x32323232,
	(uint)0xa6a6a6a6, (uint)0xc2c2c2c2, (uint)0x23232323, (uint)0x3d3d3d3d,
	(uint)0xeeeeeeee, (uint)0x4c4c4c4c, (uint)0x95959595, (uint)0x0b0b0b0b,
	(uint)0x42424242, (uint)0xfafafafa, (uint)0xc3c3c3c3, (uint)0x4e4e4e4e,
	(uint)0x08080808, (uint)0x2e2e2e2e, (uint)0xa1a1a1a1, (uint)0x66666666,
	(uint)0x28282828, (uint)0xd9d9d9d9, (uint)0x24242424, (uint)0xb2b2b2b2,
	(uint)0x76767676, (uint)0x5b5b5b5b, (uint)0xa2a2a2a2, (uint)0x49494949,
	(uint)0x6d6d6d6d, (uint)0x8b8b8b8b, (uint)0xd1d1d1d1, (uint)0x25252525,
	(uint)0x72727272, (uint)0xf8f8f8f8, (uint)0xf6f6f6f6, (uint)0x64646464,
	(uint)0x86868686, (uint)0x68686868, (uint)0x98989898, (uint)0x16161616,
	(uint)0xd4d4d4d4, (uint)0xa4a4a4a4, (uint)0x5c5c5c5c, (uint)0xcccccccc,
	(uint)0x5d5d5d5d, (uint)0x65656565, (uint)0xb6b6b6b6, (uint)0x92929292,
	(uint)0x6c6c6c6c, (uint)0x70707070, (uint)0x48484848, (uint)0x50505050,
	(uint)0xfdfdfdfd, (uint)0xedededed, (uint)0xb9b9b9b9, (uint)0xdadadada,
	(uint)0x5e5e5e5e, (uint)0x15151515, (uint)0x46464646, (uint)0x57575757,
	(uint)0xa7a7a7a7, (uint)0x8d8d8d8d, (uint)0x9d9d9d9d, (uint)0x84848484,
	(uint)0x90909090, (uint)0xd8d8d8d8, (uint)0xabababab, (uint)0x00000000,
	(uint)0x8c8c8c8c, (uint)0xbcbcbcbc, (uint)0xd3d3d3d3, (uint)0x0a0a0a0a,
	(uint)0xf7f7f7f7, (uint)0xe4e4e4e4, (uint)0x58585858, (uint)0x05050505,
	(uint)0xb8b8b8b8, (uint)0xb3b3b3b3, (uint)0x45454545, (uint)0x06060606,
	(uint)0xd0d0d0d0, (uint)0x2c2c2c2c, (uint)0x1e1e1e1e, (uint)0x8f8f8f8f,
	(uint)0xcacacaca, (uint)0x3f3f3f3f, (uint)0x0f0f0f0f, (uint)0x02020202,
	(uint)0xc1c1c1c1, (uint)0xafafafaf, (uint)0xbdbdbdbd, (uint)0x03030303,
	(uint)0x01010101, (uint)0x13131313, (uint)0x8a8a8a8a, (uint)0x6b6b6b6b,
	(uint)0x3a3a3a3a, (uint)0x91919191, (uint)0x11111111, (uint)0x41414141,
	(uint)0x4f4f4f4f, (uint)0x67676767, (uint)0xdcdcdcdc, (uint)0xeaeaeaea,
	(uint)0x97979797, (uint)0xf2f2f2f2, (uint)0xcfcfcfcf, (uint)0xcececece,
	(uint)0xf0f0f0f0, (uint)0xb4b4b4b4, (uint)0xe6e6e6e6, (uint)0x73737373,
	(uint)0x96969696, (uint)0xacacacac, (uint)0x74747474, (uint)0x22222222,
	(uint)0xe7e7e7e7, (uint)0xadadadad, (uint)0x35353535, (uint)0x85858585,
	(uint)0xe2e2e2e2, (uint)0xf9f9f9f9, (uint)0x37373737, (uint)0xe8e8e8e8,
	(uint)0x1c1c1c1c, (uint)0x75757575, (uint)0xdfdfdfdf, (uint)0x6e6e6e6e,
	(uint)0x47474747, (uint)0xf1f1f1f1, (uint)0x1a1a1a1a, (uint)0x71717171,
	(uint)0x1d1d1d1d, (uint)0x29292929, (uint)0xc5c5c5c5, (uint)0x89898989,
	(uint)0x6f6f6f6f, (uint)0xb7b7b7b7, (uint)0x62626262, (uint)0x0e0e0e0e,
	(uint)0xaaaaaaaa, (uint)0x18181818, (uint)0xbebebebe, (uint)0x1b1b1b1b,
	(uint)0xfcfcfcfc, (uint)0x56565656, (uint)0x3e3e3e3e, (uint)0x4b4b4b4b,
	(uint)0xc6c6c6c6, (uint)0xd2d2d2d2, (uint)0x79797979, (uint)0x20202020,
	(uint)0x9a9a9a9a, (uint)0xdbdbdbdb, (uint)0xc0c0c0c0, (uint)0xfefefefe,
	(uint)0x78787878, (uint)0xcdcdcdcd, (uint)0x5a5a5a5a, (uint)0xf4f4f4f4,
	(uint)0x1f1f1f1f, (uint)0xdddddddd, (uint)0xa8a8a8a8, (uint)0x33333333,
	(uint)0x88888888, (uint)0x07070707, (uint)0xc7c7c7c7, (uint)0x31313131,
	(uint)0xb1b1b1b1, (uint)0x12121212, (uint)0x10101010, (uint)0x59595959,
	(uint)0x27272727, (uint)0x80808080, (uint)0xecececec, (uint)0x5f5f5f5f,
	(uint)0x60606060, (uint)0x51515151, (uint)0x7f7f7f7f, (uint)0xa9a9a9a9,
	(uint)0x19191919, (uint)0xb5b5b5b5, (uint)0x4a4a4a4a, (uint)0x0d0d0d0d,
	(uint)0x2d2d2d2d, (uint)0xe5e5e5e5, (uint)0x7a7a7a7a, (uint)0x9f9f9f9f,
	(uint)0x93939393, (uint)0xc9c9c9c9, (uint)0x9c9c9c9c, (uint)0xefefefef,
	(uint)0xa0a0a0a0, (uint)0xe0e0e0e0, (uint)0x3b3b3b3b, (uint)0x4d4d4d4d,
	(uint)0xaeaeaeae, (uint)0x2a2a2a2a, (uint)0xf5f5f5f5, (uint)0xb0b0b0b0,
	(uint)0xc8c8c8c8, (uint)0xebebebeb, (uint)0xbbbbbbbb, (uint)0x3c3c3c3c,
	(uint)0x83838383, (uint)0x53535353, (uint)0x99999999, (uint)0x61616161,
	(uint)0x17171717, (uint)0x2b2b2b2b, (uint)0x04040404, (uint)0x7e7e7e7e,
	(uint)0xbabababa, (uint)0x77777777, (uint)0xd6d6d6d6, (uint)0x26262626,
	(uint)0xe1e1e1e1, (uint)0x69696969, (uint)0x14141414, (uint)0x63636363,
	(uint)0x55555555, (uint)0x21212121, (uint)0x0c0c0c0c, (uint)0x7d7d7d7d,
};
static const u32 rcon[] = {
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x1B000000, 0x36000000, /* for 128-bit blocks, Rijndael never uses more than 10 rcon values */
};

#define SWAP(x) (_lrotl(x, 8)&0x00ff00ff|_lrotr(x, 8)&0xff00ff00)
#ifdef _MSC_VER
	#define GETU32(p) SWAP(*((u32 *)(p)))
	#define PUTU32(ct, st) { *((u32 *)(ct)) = SWAP((st)); }
#else
	#define GETU32(pt) (((u32)(pt)[0]<<24)^((u32)(pt)[1]<<16)^((u32)(pt)[2]<<8)^((u32)(pt)[3]))
	#define PUTU32(ct, st) { (ct)[0] = (u8)((st)>>24); (ct)[1] = (u8)((st)>>16); (ct)[2] = (u8)((st)>>8); (ct)[3] = (u8)(st); }
#endif

/**
 * Expand the cipher key into the encryption key schedule.
 *
 * @return	the number of rounds for the given cipher key size.
 */
/*
 * __db_rijndaelKeySetupEnc --
 *
 * PUBLIC: int __db_rijndaelKeySetupEnc __P((u32 *, const u8 *, int));
 *
 */
int __db_rijndaelKeySetupEnc(u32 * rk /* rk[4*(Nr + 1)] */, const u8 * cipherKey, int keyBits)
{
	int i = 0;
	u32 temp;
	rk[0] = GETU32(cipherKey     );
	rk[1] = GETU32(cipherKey+4);
	rk[2] = GETU32(cipherKey+8);
	rk[3] = GETU32(cipherKey+12);
	if(keyBits == 128) {
		for(;; ) {
			temp  = rk[3];
			rk[4] = rk[0]^
			        (Te4[(temp>>16)&0xff]&0xff000000)^
			        (Te4[(temp>>8)&0xff]&0x00ff0000)^
			        (Te4[(temp)&0xff]&0x0000ff00)^
			        (Te4[(temp>>24)       ]&0x000000ff)^
			        rcon[i];
			rk[5] = rk[1]^rk[4];
			rk[6] = rk[2]^rk[5];
			rk[7] = rk[3]^rk[6];
			if(++i == 10) {
				return 10;
			}
			rk += 4;
		}
	}
	rk[4] = GETU32(cipherKey+16);
	rk[5] = GETU32(cipherKey+20);
	if(keyBits == 192) {
		for(;; ) {
			temp = rk[ 5];
			rk[ 6] = rk[ 0]^
			         (Te4[(temp>>16)&0xff]&0xff000000)^
			         (Te4[(temp>>8)&0xff]&0x00ff0000)^
			         (Te4[(temp)&0xff]&0x0000ff00)^
			         (Te4[(temp>>24)       ]&0x000000ff)^
			         rcon[i];
			rk[ 7] = rk[ 1]^rk[ 6];
			rk[ 8] = rk[ 2]^rk[ 7];
			rk[ 9] = rk[ 3]^rk[ 8];
			if(++i == 8) {
				return 12;
			}
			rk[10] = rk[ 4]^rk[ 9];
			rk[11] = rk[ 5]^rk[10];
			rk += 6;
		}
	}
	rk[6] = GETU32(cipherKey+24);
	rk[7] = GETU32(cipherKey+28);
	if(keyBits == 256) {
		for(;; ) {
			temp = rk[ 7];
			rk[ 8] = rk[ 0]^
			         (Te4[(temp>>16)&0xff]&0xff000000)^
			         (Te4[(temp>>8)&0xff]&0x00ff0000)^
			         (Te4[(temp)&0xff]&0x0000ff00)^
			         (Te4[(temp>>24)       ]&0x000000ff)^
			         rcon[i];
			rk[ 9] = rk[ 1]^rk[ 8];
			rk[10] = rk[ 2]^rk[ 9];
			rk[11] = rk[ 3]^rk[10];
			if(++i == 7) {
				return 14;
			}
			temp = rk[11];
			rk[12] = rk[ 4]^
			         (Te4[(temp>>24)       ]&0xff000000)^
			         (Te4[(temp>>16)&0xff]&0x00ff0000)^
			         (Te4[(temp>>8)&0xff]&0x0000ff00)^
			         (Te4[(temp)&0xff]&0x000000ff);
			rk[13] = rk[ 5]^rk[12];
			rk[14] = rk[ 6]^rk[13];
			rk[15] = rk[ 7]^rk[14];

			rk += 8;
		}
	}
	return 0;
}

/**
 * Expand the cipher key into the decryption key schedule.
 *
 * @return	the number of rounds for the given cipher key size.
 */
/*
 * __db_rijndaelKeySetupDec --
 *
 * PUBLIC: int __db_rijndaelKeySetupDec __P((u32 *, const u8 *, int));
 */
int __db_rijndaelKeySetupDec(u32 * rk /* rk[4*(Nr + 1)] */, const u8 * cipherKey, int keyBits)
{
	int Nr, i, j;
	u32 temp;
	/* expand the cipher key: */
	Nr = __db_rijndaelKeySetupEnc(rk, cipherKey, keyBits);
	/* invert the order of the round keys: */
	for(i = 0, j = 4*Nr; i < j; i += 4, j -= 4) {
		temp = rk[i    ]; rk[i    ] = rk[j    ]; rk[j    ] = temp;
		temp = rk[i+1]; rk[i+1] = rk[j+1]; rk[j+1] = temp;
		temp = rk[i+2]; rk[i+2] = rk[j+2]; rk[j+2] = temp;
		temp = rk[i+3]; rk[i+3] = rk[j+3]; rk[j+3] = temp;
	}
	/* apply the inverse MixColumn transform to all round keys but the first and the last: */
	for(i = 1; i < Nr; i++) {
		rk += 4;
		rk[0] =
		        Td0[Te4[(rk[0]>>24)       ]&0xff]^
		        Td1[Te4[(rk[0]>>16)&0xff]&0xff]^
		        Td2[Te4[(rk[0]>>8)&0xff]&0xff]^
		        Td3[Te4[(rk[0])&0xff]&0xff];
		rk[1] =
		        Td0[Te4[(rk[1]>>24)       ]&0xff]^
		        Td1[Te4[(rk[1]>>16)&0xff]&0xff]^
		        Td2[Te4[(rk[1]>>8)&0xff]&0xff]^
		        Td3[Te4[(rk[1])&0xff]&0xff];
		rk[2] =
		        Td0[Te4[(rk[2]>>24)       ]&0xff]^
		        Td1[Te4[(rk[2]>>16)&0xff]&0xff]^
		        Td2[Te4[(rk[2]>>8)&0xff]&0xff]^
		        Td3[Te4[(rk[2])&0xff]&0xff];
		rk[3] =
		        Td0[Te4[(rk[3]>>24)       ]&0xff]^
		        Td1[Te4[(rk[3]>>16)&0xff]&0xff]^
		        Td2[Te4[(rk[3]>>8)&0xff]&0xff]^
		        Td3[Te4[(rk[3])&0xff]&0xff];
	}
	return Nr;
}
/*
 * __db_rijndaelEncrypt --
 *
 * PUBLIC: void __db_rijndaelEncrypt __P((u32 *, int, const u8 *, u8 *));
 */
void __db_rijndaelEncrypt(u32 * rk /* rk[4*(Nr + 1)] */, int Nr, const u8 * pt, u8 * ct)
{
	u32 s0, s1, s2, s3, t0, t1, t2, t3;
#ifndef FULL_UNROLL
	int r;
#endif /* ?FULL_UNROLL */

	/*
	 * map byte array block to cipher state
	 * and add initial round key:
	 */
	s0 = GETU32(pt     )^rk[0];
	s1 = GETU32(pt+4)^rk[1];
	s2 = GETU32(pt+8)^rk[2];
	s3 = GETU32(pt+12)^rk[3];
#ifdef FULL_UNROLL
	/* round 1: */
	t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>>8)&0xff]^Te3[s3&0xff]^rk[ 4];
	t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>>8)&0xff]^Te3[s0&0xff]^rk[ 5];
	t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>>8)&0xff]^Te3[s1&0xff]^rk[ 6];
	t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>>8)&0xff]^Te3[s2&0xff]^rk[ 7];
	/* round 2: */
	s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>>8)&0xff]^Te3[t3&0xff]^rk[ 8];
	s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>>8)&0xff]^Te3[t0&0xff]^rk[ 9];
	s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>>8)&0xff]^Te3[t1&0xff]^rk[10];
	s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>>8)&0xff]^Te3[t2&0xff]^rk[11];
	/* round 3: */
	t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>>8)&0xff]^Te3[s3&0xff]^rk[12];
	t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>>8)&0xff]^Te3[s0&0xff]^rk[13];
	t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>>8)&0xff]^Te3[s1&0xff]^rk[14];
	t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>>8)&0xff]^Te3[s2&0xff]^rk[15];
	/* round 4: */
	s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>>8)&0xff]^Te3[t3&0xff]^rk[16];
	s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>>8)&0xff]^Te3[t0&0xff]^rk[17];
	s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>>8)&0xff]^Te3[t1&0xff]^rk[18];
	s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>>8)&0xff]^Te3[t2&0xff]^rk[19];
	/* round 5: */
	t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>>8)&0xff]^Te3[s3&0xff]^rk[20];
	t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>>8)&0xff]^Te3[s0&0xff]^rk[21];
	t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>>8)&0xff]^Te3[s1&0xff]^rk[22];
	t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>>8)&0xff]^Te3[s2&0xff]^rk[23];
	/* round 6: */
	s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>>8)&0xff]^Te3[t3&0xff]^rk[24];
	s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>>8)&0xff]^Te3[t0&0xff]^rk[25];
	s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>>8)&0xff]^Te3[t1&0xff]^rk[26];
	s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>>8)&0xff]^Te3[t2&0xff]^rk[27];
	/* round 7: */
	t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>>8)&0xff]^Te3[s3&0xff]^rk[28];
	t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>>8)&0xff]^Te3[s0&0xff]^rk[29];
	t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>>8)&0xff]^Te3[s1&0xff]^rk[30];
	t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>>8)&0xff]^Te3[s2&0xff]^rk[31];
	/* round 8: */
	s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>>8)&0xff]^Te3[t3&0xff]^rk[32];
	s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>>8)&0xff]^Te3[t0&0xff]^rk[33];
	s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>>8)&0xff]^Te3[t1&0xff]^rk[34];
	s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>>8)&0xff]^Te3[t2&0xff]^rk[35];
	/* round 9: */
	t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>>8)&0xff]^Te3[s3&0xff]^rk[36];
	t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>>8)&0xff]^Te3[s0&0xff]^rk[37];
	t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>>8)&0xff]^Te3[s1&0xff]^rk[38];
	t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>>8)&0xff]^Te3[s2&0xff]^rk[39];
	if(Nr > 10) {
		/* round 10: */
		s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>>8)&0xff]^Te3[t3&0xff]^rk[40];
		s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>>8)&0xff]^Te3[t0&0xff]^rk[41];
		s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>>8)&0xff]^Te3[t1&0xff]^rk[42];
		s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>>8)&0xff]^Te3[t2&0xff]^rk[43];
		/* round 11: */
		t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>>8)&0xff]^Te3[s3&0xff]^rk[44];
		t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>>8)&0xff]^Te3[s0&0xff]^rk[45];
		t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>>8)&0xff]^Te3[s1&0xff]^rk[46];
		t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>>8)&0xff]^Te3[s2&0xff]^rk[47];
		if(Nr > 12) {
			/* round 12: */
			s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>>8)&0xff]^Te3[t3&0xff]^rk[48];
			s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>>8)&0xff]^Te3[t0&0xff]^rk[49];
			s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>>8)&0xff]^Te3[t1&0xff]^rk[50];
			s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>>8)&0xff]^Te3[t2&0xff]^rk[51];
			/* round 13: */
			t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>>8)&0xff]^Te3[s3&0xff]^rk[52];
			t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>>8)&0xff]^Te3[s0&0xff]^rk[53];
			t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>>8)&0xff]^Te3[s1&0xff]^rk[54];
			t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>>8)&0xff]^Te3[s2&0xff]^rk[55];
		}
	}
	rk += Nr<<2;
#else  /* !FULL_UNROLL */
	/*
	 * Nr - 1 full rounds:
	 */
	r = Nr>>1;
	for(;; ) {
		t0 =
		        Te0[(s0>>24)       ]^
		        Te1[(s1>>16)&0xff]^
		        Te2[(s2>>8)&0xff]^
		        Te3[(s3)&0xff]^
		        rk[4];
		t1 =
		        Te0[(s1>>24)       ]^
		        Te1[(s2>>16)&0xff]^
		        Te2[(s3>>8)&0xff]^
		        Te3[(s0)&0xff]^
		        rk[5];
		t2 =
		        Te0[(s2>>24)       ]^
		        Te1[(s3>>16)&0xff]^
		        Te2[(s0>>8)&0xff]^
		        Te3[(s1)&0xff]^
		        rk[6];
		t3 =
		        Te0[(s3>>24)       ]^
		        Te1[(s0>>16)&0xff]^
		        Te2[(s1>>8)&0xff]^
		        Te3[(s2)&0xff]^
		        rk[7];

		rk += 8;
		if(--r == 0) {
			break;
		}
		s0 =
		        Te0[(t0>>24)       ]^
		        Te1[(t1>>16)&0xff]^
		        Te2[(t2>>8)&0xff]^
		        Te3[(t3)&0xff]^
		        rk[0];
		s1 =
		        Te0[(t1>>24)       ]^
		        Te1[(t2>>16)&0xff]^
		        Te2[(t3>>8)&0xff]^
		        Te3[(t0)&0xff]^
		        rk[1];
		s2 =
		        Te0[(t2>>24)       ]^
		        Te1[(t3>>16)&0xff]^
		        Te2[(t0>>8)&0xff]^
		        Te3[(t1)&0xff]^
		        rk[2];
		s3 =
		        Te0[(t3>>24)       ]^
		        Te1[(t0>>16)&0xff]^
		        Te2[(t1>>8)&0xff]^
		        Te3[(t2)&0xff]^
		        rk[3];
	}
#endif /* ?FULL_UNROLL */
	/*
	 * apply last round and
	 * map cipher state to byte array block:
	 */
	s0 =
	        (Te4[(t0>>24)       ]&0xff000000)^
	        (Te4[(t1>>16)&0xff]&0x00ff0000)^
	        (Te4[(t2>>8)&0xff]&0x0000ff00)^
	        (Te4[(t3)&0xff]&0x000000ff)^
	        rk[0];
	PUTU32(ct, s0);
	s1 =
	        (Te4[(t1>>24)       ]&0xff000000)^
	        (Te4[(t2>>16)&0xff]&0x00ff0000)^
	        (Te4[(t3>>8)&0xff]&0x0000ff00)^
	        (Te4[(t0)&0xff]&0x000000ff)^
	        rk[1];
	PUTU32(ct+4, s1);
	s2 =
	        (Te4[(t2>>24)       ]&0xff000000)^
	        (Te4[(t3>>16)&0xff]&0x00ff0000)^
	        (Te4[(t0>>8)&0xff]&0x0000ff00)^
	        (Te4[(t1)&0xff]&0x000000ff)^
	        rk[2];
	PUTU32(ct+8, s2);
	s3 =
	        (Te4[(t3>>24)       ]&0xff000000)^
	        (Te4[(t0>>16)&0xff]&0x00ff0000)^
	        (Te4[(t1>>8)&0xff]&0x0000ff00)^
	        (Te4[(t2)&0xff]&0x000000ff)^
	        rk[3];
	PUTU32(ct+12, s3);
}
/*
 * __db_rijndaelDecrypt --
 *
 * PUBLIC: void __db_rijndaelDecrypt __P((u32 *, int, const u8 *, u8 *));
 */
void __db_rijndaelDecrypt(u32 * rk /* rk[4*(Nr + 1)] */, int Nr, const u8 * ct, u8 * pt)
{
	u32 s0, s1, s2, s3, t0, t1, t2, t3;
#ifndef FULL_UNROLL
	int r;
#endif /* ?FULL_UNROLL */

	/*
	 * map byte array block to cipher state
	 * and add initial round key:
	 */
	s0 = GETU32(ct     )^rk[0];
	s1 = GETU32(ct+4)^rk[1];
	s2 = GETU32(ct+8)^rk[2];
	s3 = GETU32(ct+12)^rk[3];
#ifdef FULL_UNROLL
	/* round 1: */
	t0 = Td0[s0>>24]^Td1[(s3>>16)&0xff]^Td2[(s2>>8)&0xff]^Td3[s1&0xff]^rk[ 4];
	t1 = Td0[s1>>24]^Td1[(s0>>16)&0xff]^Td2[(s3>>8)&0xff]^Td3[s2&0xff]^rk[ 5];
	t2 = Td0[s2>>24]^Td1[(s1>>16)&0xff]^Td2[(s0>>8)&0xff]^Td3[s3&0xff]^rk[ 6];
	t3 = Td0[s3>>24]^Td1[(s2>>16)&0xff]^Td2[(s1>>8)&0xff]^Td3[s0&0xff]^rk[ 7];
	/* round 2: */
	s0 = Td0[t0>>24]^Td1[(t3>>16)&0xff]^Td2[(t2>>8)&0xff]^Td3[t1&0xff]^rk[ 8];
	s1 = Td0[t1>>24]^Td1[(t0>>16)&0xff]^Td2[(t3>>8)&0xff]^Td3[t2&0xff]^rk[ 9];
	s2 = Td0[t2>>24]^Td1[(t1>>16)&0xff]^Td2[(t0>>8)&0xff]^Td3[t3&0xff]^rk[10];
	s3 = Td0[t3>>24]^Td1[(t2>>16)&0xff]^Td2[(t1>>8)&0xff]^Td3[t0&0xff]^rk[11];
	/* round 3: */
	t0 = Td0[s0>>24]^Td1[(s3>>16)&0xff]^Td2[(s2>>8)&0xff]^Td3[s1&0xff]^rk[12];
	t1 = Td0[s1>>24]^Td1[(s0>>16)&0xff]^Td2[(s3>>8)&0xff]^Td3[s2&0xff]^rk[13];
	t2 = Td0[s2>>24]^Td1[(s1>>16)&0xff]^Td2[(s0>>8)&0xff]^Td3[s3&0xff]^rk[14];
	t3 = Td0[s3>>24]^Td1[(s2>>16)&0xff]^Td2[(s1>>8)&0xff]^Td3[s0&0xff]^rk[15];
	/* round 4: */
	s0 = Td0[t0>>24]^Td1[(t3>>16)&0xff]^Td2[(t2>>8)&0xff]^Td3[t1&0xff]^rk[16];
	s1 = Td0[t1>>24]^Td1[(t0>>16)&0xff]^Td2[(t3>>8)&0xff]^Td3[t2&0xff]^rk[17];
	s2 = Td0[t2>>24]^Td1[(t1>>16)&0xff]^Td2[(t0>>8)&0xff]^Td3[t3&0xff]^rk[18];
	s3 = Td0[t3>>24]^Td1[(t2>>16)&0xff]^Td2[(t1>>8)&0xff]^Td3[t0&0xff]^rk[19];
	/* round 5: */
	t0 = Td0[s0>>24]^Td1[(s3>>16)&0xff]^Td2[(s2>>8)&0xff]^Td3[s1&0xff]^rk[20];
	t1 = Td0[s1>>24]^Td1[(s0>>16)&0xff]^Td2[(s3>>8)&0xff]^Td3[s2&0xff]^rk[21];
	t2 = Td0[s2>>24]^Td1[(s1>>16)&0xff]^Td2[(s0>>8)&0xff]^Td3[s3&0xff]^rk[22];
	t3 = Td0[s3>>24]^Td1[(s2>>16)&0xff]^Td2[(s1>>8)&0xff]^Td3[s0&0xff]^rk[23];
	/* round 6: */
	s0 = Td0[t0>>24]^Td1[(t3>>16)&0xff]^Td2[(t2>>8)&0xff]^Td3[t1&0xff]^rk[24];
	s1 = Td0[t1>>24]^Td1[(t0>>16)&0xff]^Td2[(t3>>8)&0xff]^Td3[t2&0xff]^rk[25];
	s2 = Td0[t2>>24]^Td1[(t1>>16)&0xff]^Td2[(t0>>8)&0xff]^Td3[t3&0xff]^rk[26];
	s3 = Td0[t3>>24]^Td1[(t2>>16)&0xff]^Td2[(t1>>8)&0xff]^Td3[t0&0xff]^rk[27];
	/* round 7: */
	t0 = Td0[s0>>24]^Td1[(s3>>16)&0xff]^Td2[(s2>>8)&0xff]^Td3[s1&0xff]^rk[28];
	t1 = Td0[s1>>24]^Td1[(s0>>16)&0xff]^Td2[(s3>>8)&0xff]^Td3[s2&0xff]^rk[29];
	t2 = Td0[s2>>24]^Td1[(s1>>16)&0xff]^Td2[(s0>>8)&0xff]^Td3[s3&0xff]^rk[30];
	t3 = Td0[s3>>24]^Td1[(s2>>16)&0xff]^Td2[(s1>>8)&0xff]^Td3[s0&0xff]^rk[31];
	/* round 8: */
	s0 = Td0[t0>>24]^Td1[(t3>>16)&0xff]^Td2[(t2>>8)&0xff]^Td3[t1&0xff]^rk[32];
	s1 = Td0[t1>>24]^Td1[(t0>>16)&0xff]^Td2[(t3>>8)&0xff]^Td3[t2&0xff]^rk[33];
	s2 = Td0[t2>>24]^Td1[(t1>>16)&0xff]^Td2[(t0>>8)&0xff]^Td3[t3&0xff]^rk[34];
	s3 = Td0[t3>>24]^Td1[(t2>>16)&0xff]^Td2[(t1>>8)&0xff]^Td3[t0&0xff]^rk[35];
	/* round 9: */
	t0 = Td0[s0>>24]^Td1[(s3>>16)&0xff]^Td2[(s2>>8)&0xff]^Td3[s1&0xff]^rk[36];
	t1 = Td0[s1>>24]^Td1[(s0>>16)&0xff]^Td2[(s3>>8)&0xff]^Td3[s2&0xff]^rk[37];
	t2 = Td0[s2>>24]^Td1[(s1>>16)&0xff]^Td2[(s0>>8)&0xff]^Td3[s3&0xff]^rk[38];
	t3 = Td0[s3>>24]^Td1[(s2>>16)&0xff]^Td2[(s1>>8)&0xff]^Td3[s0&0xff]^rk[39];
	if(Nr > 10) {
		/* round 10: */
		s0 = Td0[t0>>24]^Td1[(t3>>16)&0xff]^Td2[(t2>>8)&0xff]^Td3[t1&0xff]^rk[40];
		s1 = Td0[t1>>24]^Td1[(t0>>16)&0xff]^Td2[(t3>>8)&0xff]^Td3[t2&0xff]^rk[41];
		s2 = Td0[t2>>24]^Td1[(t1>>16)&0xff]^Td2[(t0>>8)&0xff]^Td3[t3&0xff]^rk[42];
		s3 = Td0[t3>>24]^Td1[(t2>>16)&0xff]^Td2[(t1>>8)&0xff]^Td3[t0&0xff]^rk[43];
		/* round 11: */
		t0 = Td0[s0>>24]^Td1[(s3>>16)&0xff]^Td2[(s2>>8)&0xff]^Td3[s1&0xff]^rk[44];
		t1 = Td0[s1>>24]^Td1[(s0>>16)&0xff]^Td2[(s3>>8)&0xff]^Td3[s2&0xff]^rk[45];
		t2 = Td0[s2>>24]^Td1[(s1>>16)&0xff]^Td2[(s0>>8)&0xff]^Td3[s3&0xff]^rk[46];
		t3 = Td0[s3>>24]^Td1[(s2>>16)&0xff]^Td2[(s1>>8)&0xff]^Td3[s0&0xff]^rk[47];
		if(Nr > 12) {
			/* round 12: */
			s0 = Td0[t0>>24]^Td1[(t3>>16)&0xff]^Td2[(t2>>8)&0xff]^Td3[t1&0xff]^rk[48];
			s1 = Td0[t1>>24]^Td1[(t0>>16)&0xff]^Td2[(t3>>8)&0xff]^Td3[t2&0xff]^rk[49];
			s2 = Td0[t2>>24]^Td1[(t1>>16)&0xff]^Td2[(t0>>8)&0xff]^Td3[t3&0xff]^rk[50];
			s3 = Td0[t3>>24]^Td1[(t2>>16)&0xff]^Td2[(t1>>8)&0xff]^Td3[t0&0xff]^rk[51];
			/* round 13: */
			t0 = Td0[s0>>24]^Td1[(s3>>16)&0xff]^Td2[(s2>>8)&0xff]^Td3[s1&0xff]^rk[52];
			t1 = Td0[s1>>24]^Td1[(s0>>16)&0xff]^Td2[(s3>>8)&0xff]^Td3[s2&0xff]^rk[53];
			t2 = Td0[s2>>24]^Td1[(s1>>16)&0xff]^Td2[(s0>>8)&0xff]^Td3[s3&0xff]^rk[54];
			t3 = Td0[s3>>24]^Td1[(s2>>16)&0xff]^Td2[(s1>>8)&0xff]^Td3[s0&0xff]^rk[55];
		}
	}
	rk += Nr<<2;
#else  /* !FULL_UNROLL */
	/*
	 * Nr - 1 full rounds:
	 */
	r = Nr>>1;
	for(;; ) {
		t0 =
		        Td0[(s0>>24)       ]^
		        Td1[(s3>>16)&0xff]^
		        Td2[(s2>>8)&0xff]^
		        Td3[(s1)&0xff]^
		        rk[4];
		t1 =
		        Td0[(s1>>24)       ]^
		        Td1[(s0>>16)&0xff]^
		        Td2[(s3>>8)&0xff]^
		        Td3[(s2)&0xff]^
		        rk[5];
		t2 =
		        Td0[(s2>>24)       ]^
		        Td1[(s1>>16)&0xff]^
		        Td2[(s0>>8)&0xff]^
		        Td3[(s3)&0xff]^
		        rk[6];
		t3 =
		        Td0[(s3>>24)       ]^
		        Td1[(s2>>16)&0xff]^
		        Td2[(s1>>8)&0xff]^
		        Td3[(s0)&0xff]^
		        rk[7];

		rk += 8;
		if(--r == 0) {
			break;
		}
		s0 =
		        Td0[(t0>>24)       ]^
		        Td1[(t3>>16)&0xff]^
		        Td2[(t2>>8)&0xff]^
		        Td3[(t1)&0xff]^
		        rk[0];
		s1 =
		        Td0[(t1>>24)       ]^
		        Td1[(t0>>16)&0xff]^
		        Td2[(t3>>8)&0xff]^
		        Td3[(t2)&0xff]^
		        rk[1];
		s2 =
		        Td0[(t2>>24)       ]^
		        Td1[(t1>>16)&0xff]^
		        Td2[(t0>>8)&0xff]^
		        Td3[(t3)&0xff]^
		        rk[2];
		s3 =
		        Td0[(t3>>24)       ]^
		        Td1[(t2>>16)&0xff]^
		        Td2[(t1>>8)&0xff]^
		        Td3[(t0)&0xff]^
		        rk[3];
	}
#endif /* ?FULL_UNROLL */
	/*
	 * apply last round and
	 * map cipher state to byte array block:
	 */
	s0 =
	        (Td4[(t0>>24)       ]&0xff000000)^
	        (Td4[(t3>>16)&0xff]&0x00ff0000)^
	        (Td4[(t2>>8)&0xff]&0x0000ff00)^
	        (Td4[(t1)&0xff]&0x000000ff)^
	        rk[0];
	PUTU32(pt, s0);
	s1 =
	        (Td4[(t1>>24)       ]&0xff000000)^
	        (Td4[(t0>>16)&0xff]&0x00ff0000)^
	        (Td4[(t3>>8)&0xff]&0x0000ff00)^
	        (Td4[(t2)&0xff]&0x000000ff)^
	        rk[1];
	PUTU32(pt+4, s1);
	s2 =
	        (Td4[(t2>>24)       ]&0xff000000)^
	        (Td4[(t1>>16)&0xff]&0x00ff0000)^
	        (Td4[(t0>>8)&0xff]&0x0000ff00)^
	        (Td4[(t3)&0xff]&0x000000ff)^
	        rk[2];
	PUTU32(pt+8, s2);
	s3 =
	        (Td4[(t3>>24)       ]&0xff000000)^
	        (Td4[(t2>>16)&0xff]&0x00ff0000)^
	        (Td4[(t1>>8)&0xff]&0x0000ff00)^
	        (Td4[(t0)&0xff]&0x000000ff)^
	        rk[3];
	PUTU32(pt+12, s3);
}

#ifdef INTERMEDIATE_VALUE_KAT

/*
 * __db_rijndaelEncryptRound --
 *
 * PUBLIC: void __db_rijndaelEncryptRound __P((const u32 *, int, u8 *, int));
 */
void __db_rijndaelEncryptRound(rk /* rk[4*(Nr + 1)] */, Nr, pt, ct)
const u32*rk;
int Nr;
u8 * block;
int rounds;
{
	int r;
	u32 t0, t1, t2, t3;
	/*
	 * map byte array block to cipher state
	 * and add initial round key:
	 */
	u32 s0 = GETU32(block)^rk[0];
	u32 s1 = GETU32(block+4)^rk[1];
	u32 s2 = GETU32(block+8)^rk[2];
	u32 s3 = GETU32(block+12)^rk[3];
	rk += 4;

	/*
	 * Nr - 1 full rounds:
	 */
	for(r = (rounds < Nr ? rounds : Nr-1); r > 0; r--) {
		t0 =
		        Te0[(s0>>24)       ]^
		        Te1[(s1>>16)&0xff]^
		        Te2[(s2>>8)&0xff]^
		        Te3[(s3)&0xff]^
		        rk[0];
		t1 =
		        Te0[(s1>>24)       ]^
		        Te1[(s2>>16)&0xff]^
		        Te2[(s3>>8)&0xff]^
		        Te3[(s0)&0xff]^
		        rk[1];
		t2 =
		        Te0[(s2>>24)       ]^
		        Te1[(s3>>16)&0xff]^
		        Te2[(s0>>8)&0xff]^
		        Te3[(s1)&0xff]^
		        rk[2];
		t3 =
		        Te0[(s3>>24)       ]^
		        Te1[(s0>>16)&0xff]^
		        Te2[(s1>>8)&0xff]^
		        Te3[(s2)&0xff]^
		        rk[3];

		s0 = t0;
		s1 = t1;
		s2 = t2;
		s3 = t3;
		rk += 4;

	}
	/*
	 * apply last round and
	 * map cipher state to byte array block:
	 */
	if(rounds == Nr) {
		t0 =
		        (Te4[(s0>>24)       ]&0xff000000)^
		        (Te4[(s1>>16)&0xff]&0x00ff0000)^
		        (Te4[(s2>>8)&0xff]&0x0000ff00)^
		        (Te4[(s3)&0xff]&0x000000ff)^
		        rk[0];
		t1 =
		        (Te4[(s1>>24)       ]&0xff000000)^
		        (Te4[(s2>>16)&0xff]&0x00ff0000)^
		        (Te4[(s3>>8)&0xff]&0x0000ff00)^
		        (Te4[(s0)&0xff]&0x000000ff)^
		        rk[1];
		t2 =
		        (Te4[(s2>>24)       ]&0xff000000)^
		        (Te4[(s3>>16)&0xff]&0x00ff0000)^
		        (Te4[(s0>>8)&0xff]&0x0000ff00)^
		        (Te4[(s1)&0xff]&0x000000ff)^
		        rk[2];
		t3 =
		        (Te4[(s3>>24)       ]&0xff000000)^
		        (Te4[(s0>>16)&0xff]&0x00ff0000)^
		        (Te4[(s1>>8)&0xff]&0x0000ff00)^
		        (Te4[(s2)&0xff]&0x000000ff)^
		        rk[3];

		s0 = t0;
		s1 = t1;
		s2 = t2;
		s3 = t3;
	}
	PUTU32(block, s0);
	PUTU32(block+4, s1);
	PUTU32(block+8, s2);
	PUTU32(block+12, s3);
}
/*
 * __db_rijndaelDecryptRound --
 *
 * PUBLIC: void __db_rijndaelDecryptRound __P((const u32 *, int, u8 *, int));
 */
void __db_rijndaelDecryptRound(rk, Nr, pt, ct)
const u32*rk;           /* rk[4*(Nr + 1)] */
int Nr;
u8 * block;
int rounds;
{
	int r;
	u32 s0, s1, s2, s3, t0, t1, t2, t3;

	/*
	 * map byte array block to cipher state
	 * and add initial round key:
	 */
	s0 = GETU32(block     )^rk[0];
	s1 = GETU32(block+4)^rk[1];
	s2 = GETU32(block+8)^rk[2];
	s3 = GETU32(block+12)^rk[3];
	rk += 4;

	/*
	 * Nr - 1 full rounds:
	 */
	for(r = (rounds < Nr ? rounds : Nr)-1; r > 0; r--) {
		t0 =
		        Td0[(s0>>24)       ]^
		        Td1[(s3>>16)&0xff]^
		        Td2[(s2>>8)&0xff]^
		        Td3[(s1)&0xff]^
		        rk[0];
		t1 =
		        Td0[(s1>>24)       ]^
		        Td1[(s0>>16)&0xff]^
		        Td2[(s3>>8)&0xff]^
		        Td3[(s2)&0xff]^
		        rk[1];
		t2 =
		        Td0[(s2>>24)       ]^
		        Td1[(s1>>16)&0xff]^
		        Td2[(s0>>8)&0xff]^
		        Td3[(s3)&0xff]^
		        rk[2];
		t3 =
		        Td0[(s3>>24)       ]^
		        Td1[(s2>>16)&0xff]^
		        Td2[(s1>>8)&0xff]^
		        Td3[(s0)&0xff]^
		        rk[3];

		s0 = t0;
		s1 = t1;
		s2 = t2;
		s3 = t3;
		rk += 4;

	}
	/*
	 * complete the last round and
	 * map cipher state to byte array block:
	 */
	t0 =
	        (Td4[(s0>>24)       ]&0xff000000)^
	        (Td4[(s3>>16)&0xff]&0x00ff0000)^
	        (Td4[(s2>>8)&0xff]&0x0000ff00)^
	        (Td4[(s1)&0xff]&0x000000ff);
	t1 =
	        (Td4[(s1>>24)       ]&0xff000000)^
	        (Td4[(s0>>16)&0xff]&0x00ff0000)^
	        (Td4[(s3>>8)&0xff]&0x0000ff00)^
	        (Td4[(s2)&0xff]&0x000000ff);
	t2 =
	        (Td4[(s2>>24)       ]&0xff000000)^
	        (Td4[(s1>>16)&0xff]&0x00ff0000)^
	        (Td4[(s0>>8)&0xff]&0x0000ff00)^
	        (Td4[(s3)&0xff]&0x000000ff);
	t3 =
	        (Td4[(s3>>24)       ]&0xff000000)^
	        (Td4[(s2>>16)&0xff]&0x00ff0000)^
	        (Td4[(s1>>8)&0xff]&0x0000ff00)^
	        (Td4[(s0)&0xff]&0x000000ff);
	if(rounds == Nr) {
		t0 ^= rk[0];
		t1 ^= rk[1];
		t2 ^= rk[2];
		t3 ^= rk[3];
	}
	PUTU32(block, t0);
	PUTU32(block+4, t1);
	PUTU32(block+8, t2);
	PUTU32(block+12, t3);
}

#endif /* INTERMEDIATE_VALUE_KAT */
