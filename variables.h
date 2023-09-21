#pragma once
#include <bitset>
#include <map>

namespace ZeroLogic {

	// xxxxxx-xxxxxx-x-xxx
	// xxxxxx (origin) xxxxxx (destination) x (if promotion, capture) xxx (010 castles, 001 en passant, 011 promotion q, 101 promotion r, 110 promotion b, 111 promotion n, 000 normal move no capture, 100 normal move capture)
	// if castles: xx (11 wk, 10 wq, 01 bk, 00 bq)
	typedef uint16_t Move;
	typedef uint64_t Bitboard;

	static Bitboard ZobristKeys[781]{ 9664795970712590336, 18297641194476670976, 9580570031479881728, 5365209237406590976, 2248705686165380096, 5646313616825794560, 10111812896781494272, 16964360990705426432, 16782144029641093120, 9223372036854775808, 15803764893836607488, 16931496432886792192, 18204345122775373824, 8612865933169308672, 7866256932469080064, 10854744283884126208, 17694377102721323008, 3809058565380580352, 126190597526927040, 5132805769007006720, 18122302983192829952, 18391915126145501184, 3247836742336258048, 5289416916230028288, 15176307531894540288, 2076367944439685120, 885267526778438912, 7298986241360693248, 13794241418347485184, 13434258701544144896, 9636952966515343360, 4373000046300993536, 3551754561307251200, 3472743813975514112, 11884421808972824576, 1003471718912820736, 490659705973944576, 13613824184242331648, 9558526379036289024, 6225415630139805696, 11988175854780000256, 16273812364742758400, 7619287109908840448, 17288670560645050368, 203437622783250624, 11571376515677462528, 13354343204836761600, 16481887057906073600, 16736531902275727360, 17551831479157469184, 12172443424701755392, 9223372036854775808, 10518167978204921856, 11377447371691202560, 6065739456366703616, 9223372036854775808, 9223372036854775808, 11030263948765155328, 11919210290848550912, 11174543372226719744, 9472914367781142528, 5800812110582788096, 8576978018623520768, 13414730612327716864, 9223372036854775808, 16898894775352997888, 803497254955947264, 10497328160529287168, 16415526267843899392, 9223372036854775808, 3078210366170084352, 935710600685667072, 5982838724749389824, 10657539445992480768, 2922542878012723712, 3364061027370859520, 1167789521511095808, 1931992593717709824, 7118407811594024960, 13911440557888712704, 3778239891321692160, 9417927330715594752, 2963631053745049088, 7945555581965529088, 5083220121492360192, 10655482542474260480, 3236917307582882816, 15148662510507503616, 1864665166972989440, 12285354908666830848, 531169930779630528, 10411054496244580352, 2129881943535773184, 14465453778817329152, 13945676697516138496, 7767180029083732992, 3044920172118127104, 16139697858742300672, 2848130992537696256, 7362332024441165824, 9898386045374488576, 13181393746346434560, 12935529565438347264, 7713619662853875712, 14417481866371989504, 4319740935129979904, 5781519050778773504, 12554099684500525056, 9223372036854775808, 10946898726209241088, 3085053886057209856, 3817848268313988096, 12939224547810590720, 15900295677178150912, 2079789158555919360, 17372566372965871616, 11163991029544996864, 9223372036854775808, 9274557909617283072, 4452752767136971776, 16684301612843982848, 13589126288430819328, 15990683930355941376, 16992289961233842176, 14044870720969711616, 13097036937696569344, 4509874497221211648, 5166250519775352832, 13786574553329199104, 10065376907223392256, 13087279106178932736, 9223372036854775808, 5040981391380310016, 6393308279603595264, 2021300996956455424, 16058173659433607168, 197167885743813504, 11263251791538188288, 12256975842796376064, 11681771245569980416, 4631115481448209408, 6865294650308292608, 13712519488971649024, 9223372036854775808, 9223372036854775808, 7896851848826527744, 1906091350820235520, 10027400494608879616, 1748956778252187136, 6350882467739697152, 9624069210698317824, 3214700036968038400, 15823837458389522432, 15648733828320520192, 18363464309469208576, 3318390203877999616, 9223372036854775808, 13033826498936342528, 10441112831816048640, 14780482078376611840, 17547967331911745536, 12875405959218606080, 8210333990530562048, 9087548966172061696, 2050591328290124032, 1333630474116489216, 16803418376086847488, 10187083138928742400, 13203611000331726848, 16727495072379940864, 9215964039083505664, 1868165204358022144, 4801550355303191552, 17100654012799375360, 3042756249907342336, 16325638365847003136, 8297839502475370496, 5151812148923857920, 15223531438045519872, 14216503819237804032, 3564447716944352256, 243055138707805056, 8838057792630394880, 6023528660315200512, 11713619926223525888, 3031221582102778368, 3076335134683431936, 17991797981897609216, 14110722676510875648, 9163244797071639552, 12151796067846246400, 3690381419051528192, 4163472329917891584, 11976885980699838464, 12466465901965643776, 4750668960435985408, 9590478238656794624, 6364828127700753408, 17971461885660459008, 13134463841428946944, 12941406088875479040, 1660660406382042112, 4785888002202253312, 13347391688849612800, 10642774973645197312, 5517351888262571008, 4937386079628866560, 16417592447540287488, 17288020228903962624, 14451250020523450368, 5807511333861322752, 17328734717510025216, 5363925139780271104, 13073701504279349248, 11212785400371163136, 16867080394233692160, 4715373512623286272, 17595125386173227008, 4291385109613115904, 16268886832071608320, 1720833308315935744, 2088639135574640128, 10064530048149893120, 144234082633099968, 3618799246749173760, 16145594177092931584, 9223372036854775808, 15151206796170649600, 4905154723509194752, 10400837033511882752, 3492240629916451840, 11394198065905958912, 10631531343104335872, 9439549093338574848, 9223372036854775808, 11468087402538463232, 16782197083390752768, 13165758481942226944, 4083039378260460544, 4298021681991615488, 12555493499426246656, 10246609091690481664, 11714637644832071680, 13795856314246721536, 13235412390254346240, 9223372036854775808, 7326696520912822272, 8255312593586019328, 18043513812752070656, 9452293234476736512, 4418413059374017536, 5613622542608025600, 13823433384480018432, 11918823151466262528, 12549853627388133376, 15261543606504929280, 10765410641075218432, 5263069536608153600, 18071881039703547904, 1756087380667969024, 8782901758799048704, 17842724962895474688, 8124629573777981440, 15093713865231931392, 15231827837902723072, 13749855522958436352, 17801732691285843968, 9399721656656592896, 2722821460906856960, 9223372036854775808, 16105264499559215104, 653794767519772288, 7964431234935480320, 9223372036854775808, 5243036481288665088, 15555827128268746752, 7356291132198260736, 8629025182886786048, 7675308748891701248, 18168362638287986688, 14268755673660354560, 1554044709130914048, 8165381323018203136, 12365779158212726784, 4783365127332264960, 6073530745212674048, 5554108106165354496, 9223372036854775808, 12163430820853067776, 2714433099525269504, 4649052660677408768, 17005468567158898688, 3672125982354823168, 18061591939573553152, 11608157803540639744, 12554456488292945920, 17549757094814992384, 15342741650354581504, 12074888625503416320, 6227926858867400704, 3048096427227880960, 10622992179431307264, 6923748062148261888, 2050916581851631872, 3886253138287877120, 1511371370531902976, 6617178051388192768, 11655878569248454656, 17213155550072279040, 12595172856852893696, 4168061687545071616, 14948030571413569536, 9223372036854775808, 13316047604804945920, 7026254669090522112, 2184128555721097216, 10487232085195337728, 7839852384918354944, 8087460839954710528, 15719873977652514816, 10950724929438552064, 2235063083819409152, 383047878238029952, 7179563240505604096, 15523905584970156032, 12225821801993883648, 13725497349745473536, 12384848415466018816, 6926647723001001984, 6645879784143197184, 1139216436620454400, 4242138309560897536, 8234206265234799616, 9188921966180929536, 1231691419378320128, 956388778501144704, 16440316676376172544, 7861056136811611136, 7206963103582466048, 8078985657334011904, 12914315879451136000, 767278677297871872, 16703716713398233088, 5055362536671730688, 17456464235607642112, 9223372036854775808, 9522257079797981184, 2045293076240214528, 16607581837630353408, 338654087588017792, 9223372036854775808, 10007085118739922944, 4075318211172170752, 8617316386786381824, 7244003119005786112, 7109499828058327040, 9223372036854775808, 14543224946419548160, 5749869068692654080, 12008316085365161984, 1537931777046688256, 11709495139214903296, 9020740248664646656, 14533721947112300544, 6975053023171995648, 15157978265474578432, 677073822375190272, 15641224372075610112, 9223372036854775808, 9223372036854775808, 7907737837546187776, 16930588172429512704, 7537486547083313152, 11287268767772819456, 15673014582937899008, 2134760466556942848, 1453679526239840768, 10318522052799567872, 454076596848346624, 10713497329148219392, 9223372036854775808, 2527138374511034880, 3707262123209259008, 14556260404044941312, 9683310018255450112, 9223372036854775808, 3266535028644704768, 11360115881872105472, 11597129839071805440, 8520793445686026240, 9037412393047270400, 12151953269650235392, 14425088102670336000, 16572649098890860544, 15354736829608632320, 6808388430340433920, 2872656934136705536, 5093351596306528256, 2118826193953462784, 5378386489791516672, 6526541899878248448, 9223372036854775808, 10996666301746880512, 4439138561198649344, 14026105502571114496, 9223372036854775808, 4531535255622045184, 9687229632300490752, 202832237999231296, 1238581936774139904, 9223372036854775808, 18152308497410254848, 8132042565992001536, 7050553107121635328, 15684803843364714496, 17919503135857055744, 6013687852445306880, 1139788701784557056, 4874405775694385152, 13492531786708443136, 4386104965310188544, 7321799440964035584, 10904176925976905728, 11340932415385421824, 5121037309981848576, 12661442985634701312, 16140818137431746560, 13486868771138824192, 15710936623437516800, 12435735708206438400, 7980053181863968768, 17650323238402605056, 7494264819819470848, 8278827915081699328, 13078236951487356928, 1600254315462023680, 593218071684190976, 6698184466045835264, 7947548824483173376, 8039833268861431808, 11163834907422314496, 5236431127241889792, 6066723915052886016, 825624947670454016, 13810354387513610240, 15199199312574957568, 4514628877143321600, 17910700160877381632, 8999877636742914048, 13457123112535369728, 17315448270977077248, 15850764143181840384, 6506809384253088768, 12004284203132012544, 5677174334431711232, 14016126489695827968, 9223372036854775808, 5442556315782114304, 1971814910437941248, 12866602442595072000, 17498904073065990144, 3656642518282218496, 3041367419408238080, 12348541637198493696, 9864266630917324800, 7524703823941498880, 12724828136288563200, 9223372036854775808, 15933493965942056960, 18414304826344763392, 4608973345397686272, 13995061388514582528, 18127637278811308032, 15977190878581661696, 2408612098871271936, 16473080197114687488, 16893009310509010944, 16253823491563032576, 13176691467057139712, 9223372036854775808, 14831334042958401536, 9223372036854775808, 7538331362433513472, 472250433052653760, 5976944602445936640, 1417538441692691456, 14592804728830332928, 4837547767404257280, 12160473961265653760, 15423256844217282560, 12618763808367396864, 11463024777604456448, 5028009877945459712, 8898153340775640064, 14690542931086213120, 9223372036854775808, 13775729750690353152, 17835079082643709952, 2458810123496504320, 8750174824282148864, 6139936182160100352, 9143603615748734976, 13618664428340781056, 1937504985186767872, 8778307672002693120, 11486916263390289920, 18078566037593118720, 7414014096683128832, 4508697990746423296, 12506530144141002752, 2334096076969344000, 12944900479450062848, 11364709526126092288, 3793045769571215360, 15004495933597530112, 410759360887739008, 5400411706552040448, 5910028631247304704, 8386968522494418944, 8981909870719799296, 156780838043928128, 4176494652201309184, 4595120279509346304, 4203297145577885184, 17391930459799928832, 13051122020130058240, 17833394455331753984, 9223372036854775808, 8731384310908956672, 9223372036854775808, 9223372036854775808, 14162873504406704128, 10990395902999351296, 11132678568251592704, 13327000927575304192, 10618681424749121536, 9223372036854775808, 9223372036854775808, 303949973776864128, 8858508641851747328, 9400300731899127808, 13190958863248355328, 348151463578966272, 13219384480275195904, 5190929473788823552, 6655597508992686080, 11418662851349354496, 14823860873820868608, 2044518103416664064, 8412923494487306240, 8479324250300088320, 17319028155875381248, 187440753231107136, 4670938907275855872, 12338938232089794560, 17711927456318887936, 17970313343107514368, 16989078713337819136, 914199172047577088, 14019058006408486912, 14931490452613828608, 1354932876914920960, 6686313018664728576, 4061814195871889408, 13773923381483282432, 14129008148760821760, 12857338566399371264, 15651219576260352000, 14808958268476112896, 6996065687975311360, 5275816454415656960, 9254941214539878400, 6236164899870599168, 9223372036854775808, 15852852529199190016, 3486193109699051520, 9223372036854775808, 3611065264304787456, 6345174492680189952, 13831859797304389632, 10544839298751565824, 7080032647029934080, 2991214311783768064, 2075691970528507392, 5337505913778401280, 9223372036854775808, 6760768087049175040, 10391073164547121152, 2286276379089748480, 450637900599820160, 16925740931712673792, 13367873462289342464, 11792708927622617088, 16457798749248913408, 13559355560188962816, 10531112552455194624, 6200762713449056256, 1239551268664581376, 11791634209942781952, 10009318906589667328, 5041788623317121024, 6107420664979321856, 2271251172236088320, 18156474499961520128, 8100129268997569536, 1570507743932459520, 10718542887784318976, 1249475087117747200, 6175103134361841664, 4464321075609298944, 16916816027478003712, 2860573505645506560, 6010811545329680384, 9058817994016005120, 7892315592829929472, 4685678728788118528, 2414900959031762944, 4334659554223596544, 10573837635487580160, 14101437173724379136, 15888184672411054080, 18229522526489974784, 15030352113869926400, 13438798656931536896, 1989081566734212608, 6562592804292683776, 13739738287757692928, 16785212757037103104, 12545117024342036480, 11493128087665600512, 16726722161663008768, 11189069208347226112, 11408256442293972992, 13968028480031125504, 18152684185771651072, 7098981818362357760, 260363335656248832, 14173903789015490560, 5812686339294980096, 14163421174298968064, 8980019291603721216, 2586378684994520064, 3887693824493710336, 200476616421872416, 15865955653217370112, 925508846863282176, 25907401504947580, 12360079527961028608, 15454013440512702464, 7008527640097095680, 4190491122872546304, 3662372060968762368, 11400933015959001088, 6139507517756125184, 17726153752828309504, 11589704923581218816, 3020988980792166400, 7424063846982445056, 6600261039107670016, 14393930389422305280, 11771030266988343296, 2203845580922458368, 16465653503625105408, 4382232476904865280, 12500418330505658368, 16826748554823872512, 15670489571554037760, 14829922353751293952, 11077072422228824064, 15795246402203262976, 16924460857284030464, 11747321792733429760, 3608440708058863616, 15607387783493173248, 2928443932224468992, 10669247044753543168, 4372445091363282944, 12284897390897258496, 16160483735220920320, 5282632682900164608, 16653147429904025600, 10952314526351405056, 1505076530996396032, 9223372036854775808, 6333122614905499648, 12145485652573999104, 14261865974244526080, 16330786746996494336, 9223372036854775808, 9701693675747782656, 11468657668529197056, 350013186820517760, 6321218649778982912, 6153941910264875008, 2812737748868388352, 9223372036854775808, 16810062745457516544, 6610638659801978880, 16153711257911611392, 7821527498484324352, 18143937214063458304, 11169408464969052160, 2319732812621613056, 1047291813732780032, 1800011807851708160, 9247855252657000448, 13398855678802567168, 12476923117284986880, 12895241720232828928, 984934087146867456, 1507804513675857408, 6188316516929672192, 3165773983982048256, 8844508639327187968, 3436413386590816256, 5790622333313032192, 8389170196521043968, 2786840187115035648, 7509823402418183168, 14506103334217668608, 18315943774704125952, 9223372036854775808, 262106620444002528, 14617519702903873536, 16495759045154594816, 17196342845103943680, 7828642949901722624, 9305893095624050688, 306271827058652864, 5411982389873481728, 7927078327712069632, 7829883947796598784, 12777387368551612416, 4893224926931124224, 2697993603483940864, 9223372036854775808, 10366565498534045696, 10579579109033979904, 7656252434347804672, 5738221966423697408, 11363417153350983680, 5966632415692111872, 16466070950558998528, 9223372036854775808, 17060028777906964480, 9318684907596648448, 9108968451809730560, 7934052200029865984, 13837420629355298816, 9829172951035994112, 4958438696947969024, 5060028127912669184, 6237553777121554432, 7940834957492337664, 13976474853675716608, 14967353295579068416, 17130562229408112640, 9223372036854775808, 16125448862979874816, 6921265091085561856, 15755817294422036480, 13568616571498092544, 5858642856476997632, 18153383833698570240, 2602420467796171776, 6052231896420860928, 17165433442882164736, 4048046007791068160, 11823710705565405184, 1186705938163881984, 6125112952000974848, 5545734561015233536, 2722872402382870528, 9704811664320948224, 17055642334198298624, 12268213973078624256, 2904753064734276608, 5280644674360487936, 14980831618816600064, 9007899757375772672, 4118018325406151168 };

	enum Result {
		checkmate = 0,
		stalemate = 1
	};

	enum Piece {
		P = 0,
		R = 1,
		N = 2,
		B = 3,
		Q = 4,
		K = 5,

		wP = 0,
		wR = 1,
		wN = 2,
		wB = 3,
		wQ = 4,
		wK = 5,
		bP = 6,
		bR = 7,
		bN = 8,
		bB = 9,
		bQ = 10,
		bK = 11,

		nope = 12
	};

	enum Color : const bool {
		black = false,
		white = true
	};

	enum Square {
		h1, g1, f1, e1, d1, c1, b1, a1,
		h2, g2, f2, e2, d2, c2, b2, a2,
		h3, g3, f3, e3, d3, c3, b3, a3,
		h4, g4, f4, e4, d4, c4, b4, a4,
		h5, g5, f5, e5, d5, c5, b5, a5,
		h6, g6, f6, e6, d6, c6, b6, a6,
		h7, g7, f7, e7, d7, c7, b7, a7,
		h8, g8, f8, e8, d8, c8, b8, a8
	};

	enum Ranks : const Bitboard {
		first = 0xFF,
		second = 0xFF00,
		third = 0xFF0000,
		fourth = 0xFF000000,
		fifth = 0xFF00000000,
		sixth = 0xFF0000000000,
		seventh = 0xFF000000000000,
		eighth = 0xFF00000000000000,
		lower = first | second | third | fourth,
		upper = fifth | sixth | seventh | eighth,
		bigmid = upper >> 16
	};

	enum Files : const Bitboard {
		A = 0x8080808080808080,
		Bf = 0x4040404040404040,
		C = 0x2020202020202020,
		D = 0x1010101010101010,
		E = 0x808080808080808,
		F = 0x404040404040404,
		G = 0x202020202020202,
		H = 0x101010101010101
	};

	namespace Movegen {

		enum PType {
			none, diagonal, straight
		};

		enum Direction {
			x = 1,
			y = 8,
			z = 9,
			z2 = 7,
			null = 0
		};

		enum Borders : const Bitboard {
			north = ~0xFF00000000000000,
			south = ~0xFF,
			east = ~0x101010101010101,
			west = ~0x8080808080808080,
			northeast = ~(0xFF00000000000000 | 0x101010101010101),
			northwest = ~(0xFF00000000000000 | 0x8080808080808080),
			southeast = ~(0xFF | 0x101010101010101),
			southwest = ~(0xFF | 0x8080808080808080),


			// knight borders: up(u)/down(d)/left(l)/right(r)

			urr = ~0xFF03030303030303,
			uur = ~0xFFFF010101010101,
			drr = ~0x03030303030303FF,
			ddr = ~0x010101010101FFFF,
			ull = ~0xFFC0C0C0C0C0C0C0,
			uul = ~0xFFFF808080808080,
			dll = ~0xC0C0C0C0C0C0C0FF,
			ddl = ~0x808080808080FFFF
		};

		enum Castling : const Bitboard {
			wkCheck = 0xE,
			wqCheck = 0x38,
			bkCheck = 0xE00000000000000,
			bqCheck = 0x3800000000000000,
			wkOcc = 0x6,
			wqOcc = 0x70,
			bkOcc = 0x600000000000000,
			bqOcc = 0x7000000000000000,
			wkhere = 0x8,
			bkhere = 0x800000000000000
		};

		enum Constants : const Bitboard {
			empty = 0,
			full = 0xFFFFFFFFFFFFFFFF
		};

	}

	namespace Search {

		enum SType {
			PERFT,
			BESTMOVE
		};

		namespace Stats {

			extern int partialnodes;
			extern uint32_t overallnodes;
			extern int maxDepth;
			extern int fullDepth;

		}

	}

	namespace State {

		enum move : const Bitboard {
			wkr = 1,
			wqr = 0x80,
			bkr = 0x100000000000000,
			bqr = 0x8000000000000000,

			wkfullk = 0b1010,
			wkfullr = 0b101,
			wqfullk = 0b101000,
			wqfullr = 0b10010000,

			bkfullk = 0xA00000000000000,
			bkfullr = 0x500000000000000,
			bqfullk = 0x2800000000000000,
			bqfullr = 0x9000000000000000
		};

		enum moveflags {
			flagmask = 0b111,

			normal = 0,
			capture = 0b100,
			passant = 0b1,
			promotionq = 0b11,
			promotionr = 0b101,
			promotionb = 0b110,
			promotionn = 0b111,
			castles = 0b10,
			promotioncapture = 0b1000,

			wkflag = (3 << 14),
			wqflag = (2 << 14),
			bkflag = (1 << 14),
			bqflag = 0,

			none = 0

		};

		enum castleType {
			wk = 0,
			wq = 1,
			bk = 2,
			bq = 3,
		};

	}

	namespace Evaluation {

		enum Phase {
			opening = 0,
			midgame = 1,
			endgame = 2
		};

		enum psqt : const Bitboard {
			center = 0x3838000000,
			extendedCenter = 0x3C04043C0000,
			middleStrip = 0xFFFF000000,
			extendedStrip = 0xFFFF0000FFFF00,
			queensStarting = 0x1000000000000010
		};

		enum castleyet : const Bitboard {
			wks = 0x2,
			wqs = 0x20,
			weither = wks | wqs,
			bks = wks << 56,
			bqs = 0x20 << 56,
			beither = bks | bqs,

			wsafe = wks | (wks >> 1) | (wqs << 1) | (wqs << 2),
			bsafe = bks | (bks >> 1) | (bqs << 1) | (bqs << 2)
		};

	}
}