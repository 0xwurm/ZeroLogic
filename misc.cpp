#include "misc.h"
#include "gamestate.h"
#include <iostream>


namespace ZeroLogic {
	namespace Miscellaneous {

		const std::string numstrConversionTable[64] = { "h1", "g1", "f1", "e1", "d1", "c1", "b1", "a1", "h2", "g2", "f2", "e2", "d2", "c2", "b2", "a2", "h3", "g3", "f3", "e3", "d3", "c3", "b3", "a3", "h4", "g4", "f4", "e4", "d4", "c4", "b4", "a4", "h5", "g5", "f5", "e5", "d5", "c5", "b5", "a5", "h6", "g6", "f6", "e6", "d6", "c6", "b6", "a6", "h7", "g7", "f7", "e7", "d7", "c7", "b7", "a7", "h8", "g8", "f8", "e8", "d8", "c8", "b8", "a8" };

		std::map<char, int> strnumConversionTable1{ {'a', 7}, {'b', 6}, {'c', 5}, {'d', 4}, {'e', 3}, {'f', 2}, {'g', 1}, {'h', 0} };
		std::map<char, int> strnumConversionTable2{ {'1', 0}, {'2', 8}, {'3', 16}, {'4', 24}, {'5', 32}, {'6', 40}, {'7', 48}, {'8', 56} };

	}
}

using namespace ZeroLogic;
using namespace Miscellaneous;

std::string Misc::numToString(Move m) {
	Move flagged = m & flagmask;

	if (flagged != castles) {

		Move origin = m >> 10;
		m <<= 6;
		Move destination = m >> 10;

		if (flagged == promotionq) {
			return numstrConversionTable[origin] + numstrConversionTable[destination] + "q";
		}
		else if (flagged == promotionb) {
			return numstrConversionTable[origin] + numstrConversionTable[destination] + "b";
		}
		else if (flagged == promotionr) { 
			return numstrConversionTable[origin] + numstrConversionTable[destination] + "r";
		}
		else if (flagged == promotionn) {
			return numstrConversionTable[origin] + numstrConversionTable[destination] + "n";
		}
		else {
			return numstrConversionTable[origin] + numstrConversionTable[destination];
		}
	}
	else {
		Move castleFlag = m & wkflag;

		if (castleFlag == wkflag) {
			return "e1g1";
		}
		else if (castleFlag == bkflag) {
			return "e8g8";
		}
		else if (castleFlag == wqflag){
			return "e1c1";
		}
		else {
			return "e8c8";
		}
	}
}

Move Misc::stringToNum(std::string m, Gamestate* gs) {

	int eblackConst = (gs->white) ? 6 : 0;

	if (m == "e1g1") {
		if (gs->position[wK] & Movegen::wkhere) { return wkflag | castles; }
	}
	else if (m == "e1c1") {
		if (gs->position[wK] & Movegen::wkhere) { return wqflag | castles; }
	}
	else if (m == "e8g8") {
		if (gs->position[bK] & Movegen::bkhere) { return bkflag | castles; }
	}
	else if (m == "e8c8") {
		if (gs->position[bK] & Movegen::bkhere) { return bqflag | castles; }
	}
	Move destination = strnumConversionTable1[m[2]] + strnumConversionTable2[m[3]];
	Move origin = strnumConversionTable1[m[0]] + strnumConversionTable2[m[1]];
	Bitboard destinationmask = static_cast<Bitboard>(1) << destination;
	Bitboard originmask = static_cast<Bitboard>(1) << origin;

	bool captureb = false;
	for (int i = 0; i < 6; i++) { if (gs->position[i + eblackConst] & destinationmask) { captureb = true; break; } }

	if (!captureb) {
		if (m.size() == 5) { // meaning if move is a promotion
			if (m.back() == 'q') { return (origin << 10) | (destination << 4) | promotionq; }
			if (m.back() == 'b') { return (origin << 10) | (destination << 4) | promotionb; }
			if (m.back() == 'r') { return (origin << 10) | (destination << 4) | promotionr; }
			if (m.back() == 'n') { return (origin << 10) | (destination << 4) | promotionn; }
		}
		else {
			int blackConst = (gs->white) ? 0 : 6;
			int movedpiece = moveflags::none;
			for (int i = 0; i < 6; i++) { if (gs->position[i + blackConst] & originmask) { movedpiece = i; break; } }

			if (movedpiece == P && gs->enPassant & destinationmask) {
				return (origin << 10) | (destination << 4) | passant;
			}
			else {
				return (origin << 10) | (destination << 4) | normal;
			}
		}
	}
	else {
		if (m.size() == 5) {
			if (m.back() == 'q') { return (origin << 10) | (destination << 4) | promotioncapture | promotionq; }
			if (m.back() == 'b') { return (origin << 10) | (destination << 4) | promotioncapture | promotionb; }
			if (m.back() == 'r') { return (origin << 10) | (destination << 4) | promotioncapture | promotionr; }
			if (m.back() == 'n') { return (origin << 10) | (destination << 4) | promotioncapture | promotionn; }
		}
		else {
			return (origin << 10) | (destination << 4) | capture;
		}

	}
}

void Misc::makeZobristKeys() {

	// 781 unique & random 64bit numbers
	// 10 ^ 19 -> 64bit
	// 781 * 19 = 14839

	std::string pi14839 = "1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679821480865132823066470938446095505822317253594081284811174502841027019385211055596446229489549303819644288109756659334461284756482337867831652712019091456485669234603486104543266482133936072602491412737245870066063155881748815209209628292540917153643678925903600113305305488204665213841469519415116094330572703657595919530921861173819326117931051185480744623799627495673518857527248912279381830119491298336733624406566430860213949463952247371907021798609437027705392171762931767523846748184676694051320005681271452635608277857713427577896091736371787214684409012249534301465495853710507922796892589235420199561121290219608640344181598136297747713099605187072113499999983729780499510597317328160963185950244594553469083026425223082533446850352619311881710100031378387528865875332083814206171776691473035982534904287554687311595628638823537875937519577818577805321712268066130019278766111959092164201989380952572010654858632788659361533818279682303019520353018529689957736225994138912497217752834791315155748572424541506959508295331168617278558890750983817546374649393192550604009277016711390098488240128583616035637076601047101819429555961989467678374494482553797747268471040475346462080466842590694912933136770289891521047521620569660240580381501935112533824300355876402474964732639141992726042699227967823547816360093417216412199245863150302861829745557067498385054945885869269956909272107975093029553211653449872027559602364806654991198818347977535663698074265425278625518184175746728909777727938000816470600161452491921732172147723501414419735685481613611573525521334757418494684385233239073941433345477624168625189835694855620992192221842725502542568876717904946016534668049886272327917860857843838279679766814541009538837863609506800642251252051173929848960841284886269456042419652850222106611863067442786220391949450471237137869609563643719172874677646575739624138908658326459958133904780275900994657640789512694683983525957098258226205224894077267194782684826014769909026401363944374553050682034962524517493996514314298091906592509372216964615157098583874105978859597729754989301617539284681382686838689427741559918559252459539594310499725246808459872736446958486538367362226260991246080512438843904512441365497627807977156914359977001296160894416948685558484063534220722258284886481584560285060168427394522674676788952521385225499546667278239864565961163548862305774564980355936345681743241125150760694794510965960940252288797108931456691368672287489405601015033086179286809208747609178249385890097149096759852613655497818931297848216829989487226588048575640142704775551323796414515237462343645428584447952658678210511413547357395231134271661021359695362314429524849371871101457654035902799344037420073105785390621983874478084784896833214457138687519435064302184531910484810053706146806749192781911979399520614196634287544406437451237181921799983910159195618146751426912397489409071864942319615679452080951465502252316038819301420937621378559566389377870830390697920773467221825625996615014215030680384477345492026054146659252014974428507325186660021324340881907104863317346496514539057962685610055081066587969981635747363840525714591028970641401109712062804390397595156771577004203378699360072305587631763594218731251471205329281918261861258673215791984148488291644706095752706957220917567116722910981690915280173506712748583222871835209353965725121083579151369882091444210067510334671103141267111369908658516398315019701651511685171437657618351556508849099898599823873455283316355076479185358932261854896321329330898570642046752590709154814165498594616371802709819943099244889575712828905923233260972997120844335732654893823911932597463667305836041428138830320382490375898524374417029132765618093773444030707469211201913020330380197621101100449293215160842444859637669838952286847831235526582131449576857262433441893039686426243410773226978028073189154411010446823252716201052652272111660396665573092547110557853763466820653109896526918620564769312570586356620185581007293606598764861179104533488503461136576867532494416680396265797877185560845529654126654085306143444318586769751456614068007002378776591344017127494704205622305389945613140711270004078547332699390814546646458807972708266830634328587856983052358089330657574067954571637752542021149557615814002501262285941302164715509792592309907965473761255176567513575178296664547791745011299614890304639947132962107340437518957359614589019389713111790429782856475032031986915140287080859904801094121472213179476477726224142548545403321571853061422881375850430633217518297986622371721591607716692547487389866549494501146540628433663937900397692656721463853067360965712091807638327166416274888800786925602902284721040317211860820419000422966171196377921337575114959501566049631862947265473642523081770367515906735023507283540567040386743513622224771589150495309844489333096340878076932599397805419341447377441842631298608099888687413260472156951623965864573021631598193195167353812974167729478672422924654366800980676928238280689964004824354037014163149658979409243237896907069779422362508221688957383798623001593776471651228935786015881617557829735233446042815126272037343146531977774160319906655418763979293344195215413418994854447345673831624993419131814809277771038638773431772075456545322077709212019051660962804909263601975988281613323166636528619326686336062735676303544776280350450777235547105859548702790814356240145171806246436267945612753181340783303362542327839449753824372058353114771199260638133467768796959703098339130771098704085913374641442822772634659470474587847787201927715280731767907707157213444730605700733492436931138350493163128404251219256517980694113528013147013047816437885185290928545201165839341965621349143415956258658655705526904965209858033850722426482939728584783163057777560688876446248246857926039535277348030480290058760758251047470916439613626760449256274204208320856611906254543372131535958450687724602901618766795240616342522577195429162991930645537799140373404328752628889639958794757291746426357455254079091451357111369410911939325191076020825202618798531887705842972591677813149699009019211697173727847684726860849003377024242916513005005168323364350389517029893922334517220138128069650117844087451960121228599371623130171144484640903890644954440061986907548516026327505298349187407866808818338510228334508504860825039302133219715518430635455007668282949304137765527939751754613953984683393638304746119966538581538420568533862186725233402830871123282789212507712629463229563989898935821167456270102183564622013496715188190973038119800497340723961036854066431939509790190699639552453005450580685501956730229219139339185680344903982059551002263535361920419947455385938102343955449597783779023742161727111723643435439478221818528624085140066604433258885698670543154706965747458550332323342107301545940516553790686627333799585115625784322988273723198987571415957811196358330059408730681216028764962867446047746491599505497374256269010490377819868359381465741268049256487985561453723478673303904688383436346553794986419270563872931748723320837601123029911367938627089438799362016295154133714248928307220126901475466847653576164773794675200490757155527819653621323926406160136358155907422020203187277605277219005561484255518792530343513984425322341576233610642506390497500865627109535919465897514131034822769306247435363256916078154781811528436679570611086153315044521274739245449454236828860613408414863776700961207151249140430272538607648236341433462351897576645216413767969031495019108575984423919862916421939949072362346468441173940326591840443780513338945257423995082965912285085558215725031071257012668302402929525220118726767562204154205161841634847565169998116141010029960783869092916030288400269104140792886215078424516709087000699282120660418371806535567252532567532861291042487761825829765157959847035622262934860034158722980534989650226291748788202734209222245339856264766914905562842503912757710284027998066365825488926488025456610172967026640765590429099456815065265305371829412703369313785178609040708667114965583434347693385781711386455873678123014587687126603489139095620099393610310291616152881384379099042317473363948045759314931405297634757481193567091101377517210080315590248530906692037671922033229094334676851422144773793937517034436619910403375111735471918550464490263655128162288244625759163330391072253837421821408835086573917715096828874782656995995744906617583441375223970968340800535598491754173818839994469748676265516582765848358845314277568790029095170283529716344562129640435231176006651012412006597558512761785838292041974844236080071930457618932349229279650198751872127267507981255470958904556357921221033346697499235630254947802490114195212382815309114079073860251522742995818072471625916685451333123948049470791191532673430282441860414263639548000448002670496248201792896476697583183271314251702969234889627668440323260927524960357996469256504936818360900323809293459588970695365349406034021665443755890045632882250545255640564482465151875471196218443965825337543885690941130315095261793780029741207665147939425902989695946995565761218656196733786236256125216320862869222103274889218654364802296780705765615144632046927906821207388377814233562823608963208068222468012248261177185896381409183903673672220888321513755600372798394004152970028783076670944474560134556417254370906979396122571429894671543578468788614445812314593571984922528471605049221242470141214780573455105008019086996033027634787081081754501193071412233908663938339529425786905076431006383519834389341596131854347546495569781038293097164651438407007073604112373599843452251610507027056235266012764848308407611830130527932054274628654036036745328651057065874882256981579367897669742205750596834408697350201410206723585020072452256326513410559240190274216248439140359989535394590944070469120914093870012645600162374288021092764579310657922955249887275846101264836999892256959688159205600101655256375678566722796619885782794848855834397518744545512965634434803966420557982936804352202770984294232533022576341807039476994159791594530069752148293366555661567873640053666564165473217043903521329543529169414599041608753201868379370234888689479151071637852902345292440773659495630510074210871426134974595615138498713757047101787957310422969066670214498637464595280824369445789772330048764765241339075920434019634039114732023380715095222010682563427471646024335440051521266932493419673977041595683753555166730273900749729736354964533288869844061196496162773449518273695588220757355176651589855190986665393549481068873206859907540792342402300925900701731960362254756478940647548346647760411463233905651343306844953979070903023460461470961696886885014083470405460742958699138296682468185710318879065287036650832431974404771855678934823089431068287027228097362480939962706074726455399253994428081137369433887294063079261595995462624629707062594845569034711972996409089418059534393251236235508134949004364278527138315912568989295196427287573946914272534366941532361004537304881985517065941217352462589548730167600298865925786628561249665523533829428785425340483083307016537228563559152534784459818313411290019992059813522051173365856407826484942764411376393866924803118364453698589175442647399882284621844900877769776312795722672655562596282542765318300134070922334365779160128093179401718598599933849235495640057099558561134980252499066984233017350358044081168552653117099570899427328709258487894436460050410892266917835258707859512983441729535195378855345737426085902908176515578039059464087350612322611200937310804854852635722825768203416050484662775045003126200800799804925485346941469775164932709504934639382432227188515974054702148289711177792376122578873477188196825462981268685817050740272550263329044976277894423621674119186269439650671515779586756482399391760426017633870454990176143641204692182370764887834196896861181558158736062938603810171215855272668300823834046564758804051380801633638874216371406435495561868964112282140753302655100424104896783528588290243670904887118190909494533144218287661810310073547705498159680772009474696134360928614849417850171807793068108546900094458995279424398139213505586422196483491512639012803832001097738680662877923971801461343244572640097374257007359210031541508936793008169980536520276007277496745840028362405346037263416554259027601834840306811381855105979705664007509426087885735796037324514146786703688098806097164258497595138069309449401515422221943291302173912538355915031003330325111749156969174502714943315155885403922164097229101129035521815762823283182342548326111912800928252561902052630163911477247331485739107775874425387611746578671169414776421441111263583553871361011023267987756410246824032264834641766369806637857681349204530224081972785647198396308781543221166912246415911776732253264335686146186545222681268872684459684424161078540167681420808850280054143613146230821025941737562389942075713627516745731891894562835257044133543758575342698699472547031656613991999682628247270641336222178923903176085428943733935618891651250424404008952719837873864805847268954624388234375178852014395600571048119498842390606136957342315590796703461491434478863604103182350736502778590897578272731305048893989009923913503373250855982655867089242612429473670193907727130706869170926462548423240748550366080136046689511840093668609546325002145852930950000907151058236267293264537382104938724996699339424685516483261134146110680267446637334375340764294026682973865220935701626384648528514903629320199199688285171839536691345222444708045923966028171565515656661113598231122506289058549145097157553900243931535190902107119457300243880176615035270862602537881797519478061013715004489917210022201335013106016391541589578037117792775225978742891917915522417189585361680594741234193398420218745649256443462392531953135103311476394911995072858430658361935369329699289837914941939406085724863968836903265564364216644257607914710869984315733749648835292769328220762947282381537409961545598798259891093717126218283025848112389011968221429457667580718653806506487026133892822994972574530332838963818439447707794022843598834100358385423897354243956475556840952248445541392394100016207693636846776413017819659379971557468541946334893748439129742391433659360410035234377706588867781139498616478747140793263858738624732889645643598774667638479466504074111825658378878454858148962961273998413442726086061872455452360643153710112746809778704464094758280348769758948328241239292960582948619196670918958089833201210318430340128495116203534280144127617285830243559830032042024512072872535581195840149180969253395075778400067465526031446167050827682772223534191102634163157147406123850425845988419907611287258059113935689601431668283176323567325417073420817332230462987992804908514094790368878687894930546955703072619009502";

	for (int i = 0; i < 781; ++i) {

		for (int j = 0; j < 19; ++j) {
			ZobristKeys[i] += (pi14839[0] - 48) * pow(10, j);
			pi14839.erase(pi14839.begin());
		}

		std::cout << ZobristKeys[i] << ", ";

	}

}

void Misc::makeKingLookup() {

	for (int i = 0; i < 64; i++) {
		Bitboard king = static_cast<Bitboard>(1) << i;
		Bitboard legalMoves{};
		legalMoves |= ((king & Movegen::west) << 1);
		legalMoves |= ((king & Movegen::east) >> 1);
		legalMoves |= ((king & Movegen::north) << 8);
		legalMoves |= ((king & Movegen::south) >> 8);
		legalMoves |= ((king & Movegen::northeast) << 7);
		legalMoves |= ((king & Movegen::southwest) >> 7);
		legalMoves |= ((king & Movegen::northwest) << 9);
		legalMoves |= ((king & Movegen::southeast) >> 9);
		std::cout << legalMoves << ", ";
	}

}

void Misc::makeKnightLookup() {

	for (int i = 0; i < 64; i++) {
		Bitboard knight = static_cast<Bitboard>(1) << i;
		Bitboard legalMoves{};
		legalMoves |= ((knight & Movegen::dll) >> 6);
		legalMoves |= ((knight & Movegen::urr) << 6);
		legalMoves |= ((knight & Movegen::drr) >> 10);
		legalMoves |= ((knight & Movegen::ull) << 10);
		legalMoves |= ((knight & Movegen::ddl) >> 15);
		legalMoves |= ((knight & Movegen::uur) << 15);
		legalMoves |= ((knight & Movegen::uul) << 17);
		legalMoves |= ((knight & Movegen::ddr) >> 17);
		std::cout << legalMoves << ", ";
	}

}
