---
title: Player-tools 149 — reading edition with minds
date: 2026-08-29
tags: [session-pipeline, story, monologue, perception]
---

# The cemetery, again

A reading edition of `experiments/session_pipeline/runs/player-tools-300turn`. The public line is the saved transcript. Private sight and thought are whatever the run actually retained — every perception apply logged in `console.log` (438 lines), and every monologue line still in `world.json` (Aqua 109, Darkness 122, Megumin 112, Luna 40). Perception is an overwrite: each *sees* block is the new string as of that scene turn, not a history of all prior sights. Monologue turns are the scene `turn_index` of the storyline that character was on; a fork can reuse a number. Those lines are printed after the main-scene beat with the same number, and are marked if more than one line shares a name and turn.

The run resumed a 3-turn guild save, then played 149 more eval turns under the fork_merge guide. It stopped on DeepSeek 402 after beat 151, with Aqua forked once more (`konosuba_f180_0`). No missing private prose was written in. Empty narrator beats are omitted. The engine roster name for the player is `Player`; here he is **Kazuma**, as the cast addresses him.

Companion analysis: [player-tools-149-narrative-analysis-2026-08-29.md](player-tools-149-narrative-analysis-2026-08-29.md).

## Prologue

Another beautiful morning in Axel — which means another morning of being broke.

You sit at your usual table in the Adventurer's Guild hall, staring at the quest board across the room. The other adventurers give your table a wide berth. Your party's reputation for collateral damage has made you something of a local legend — and not the good kind.

Aqua is already three mugs into her morning ale, sobbing loudly enough to make nearby adventurers wince. "Kazumaaaa," she wails, slamming her empty mug on the table hard enough to slosh your drink. "We need money! I can't keep drinking this cheap swill! A goddess deserves at least mid-shelf!"

"We're broke BECAUSE you keep drinking," you mutter, but she's already flagging down the waitress for another round on your tab.

Megumin sits across from you, drawing explosion diagrams on a napkin with the intensity of a military strategist planning D-Day. She already went on her daily Explosion pilgrimage at dawn — you know because the distant boom rattled the mansion windows and woke you up. She's been unusable for combat since about 6 AM.

Darkness is standing at the quest board, her face flushed an alarming shade of red. She's found something. Given her expression — somewhere between terror and excitement — it probably involves monsters with tentacles.

"K-Kazuma," she stammers, not turning around. "There's a quest here for subjugating a nest of Brutal Alligators in the sewers. It says they... they grapple their prey and drag them underwater..."

She's trembling. That's not fear.

Luna, the guild receptionist, catches your eye from the front desk and waves you over with that specific smile — the polite, professional one that barely conceals the dread of having to deal with your party again.


## I. A goddess at the door

### 1

**Kazuma.** I glance at Aqua, still sobbing into her mug, then back at Darkness practically vibrating at the quest board.

"Darkness — the cemetery's got undead piling up because of her goddess aura. You and Megumin hold down the guild. Aqua." I jerk my chin at her. "Get up. You're going to the cemetery to purify the undead you've been attracting. That's your job, so go do it — and don't come back until you've actually done it."

The hall holds its breath for half a second, then lets it out in a collective, practiced sigh. Aqua's finger is still aimed at you, trembling with the force of a complaint that has been rehearsed across three mugs and many ruined mornings. Darkness's hand has not quite come back down from where she reached for the board. Megumin grips the edge of the table to steady herself, as if standing is a tactical decision she is still negotiating.

None of them move toward the door. The cemetery, somewhere beyond the guild's warm windows, keeps its dead to itself for the moment.

**Aqua.** Kazumaaaa! This is YOUR fault! I am a goddess, not some gravedigger who wades through your zombie problem! If you hadn't dragged me to this world, none of this would be happening! (on her feet, jabbing a trembling finger at the Player, ale sloshing over the rim of her mug)

**Darkness.** But Kazuma... my shield belongs at the front. If the cemetery is truly overrun, then a Crusader must stand among the graves and take every grasping hand... e-every single one... (voice dropping, one hand pressed to her chest as she imagines it)

**Megumin.** Sentry duty? For me, the foremost genius of the Crimson Demon Clan? My Explosion could cleanse that cemetery in a single, glorious incantation! ...Though I may require a moment to stand first. (gripping the table edge, chin high despite wobbling legs)


*Aqua, privately.* Fine. If shame won't move him, then I'll make this whole guild kneel with water up to their teeth. He can choke on his apology.

*Darkness, privately.* I'll take first watch. It's the proper thing—and if the dark wants to test me, at least no one will see which part of me is praying.

*Luna, privately.* Let them argue a little longer—if I set the form beside Aqua's pointing hand at the right angle, she'll sign it just to prove she was right.

*Megumin, privately.* Rise before Aqua says 'useless,' say the thing, let the corpses be the encore. If I don't, they'll remember her version.


### 2

**Kazuma.** I watch Aqua wag her finger at me, and I don't blink. I just let the silence stretch before I push my chair back and stand.

"Aqua." I grab her by the sleeve of her priestess robe and tug her toward the door. "You heard me. Cemetery. Undead. Purify. It's literally what you're for. One Healing spell and a prayer and you're a hero — stop wailing about mid-shelf and go earn it."

I point at her, then jerk my thumb at the door.

The chair scrapes back and the guild floor catches your weight. Your grip closes on the loose sleeve of Aqua's robe, and the fabric bunches as you pull her toward the door. She stumbles half a step, ale still sloshing in her mug, her free hand flying out for balance. The hall's practiced sigh thins into a murmur, then a circle of not-quite-hidden stares.

Near the quest board, Darkness lowers her hand by degrees, then raises it again as if volunteering for something terrible. Megumin stays at the table, chin high, both hands planted on the edge like a captain lashed to the mast of a listing ship. At the counter, Luna has stopped pretending to file papers; her smile is fixed in place.

You point at Aqua. The finger lands a breath from her nose. Beyond the doorway, the morning is bright enough to show every dust mote hanging in the air.

**Aqua.** Kazumaaaa! Stop dragging me like a common criminal! I am a goddess, not a mule for your zombie chores! If you hadn't dragged me to this world, none of those rotting hands would exist in the first place! (twisting in his grip, trying to free her sleeve while keeping hold of her mug)

**Darkness.** Then I shall go as well! If the goddess must wade among the dead, a Crusader's place is at her side... to catch every claw, every bite, every cold hand that reaches for her...! (one hand raised, cheeks flushed, already stepping toward the door)

**Megumin.** You intend to send our only goddess into a den of the dead without the glory of Explosion? I could turn that cemetery into a beacon seen for miles! ...Just allow me a moment to find my feet, and I shall accompany you as her rear guard. (still gripping the table, one knee wobbling as she attempts to rise)

**Luna.** The cemetery quest is the third notice from the top. The client asked for the undead to be purified... not incinerated. Please be mindful of the headstones. And the mausoleums. And the fence. (behind the counter, smiling with polite strain)


*Aqua, privately.* I'm letting my knees buckle now. Not to kneel—to make him carry a goddess, and the second he staggers I'll go for the well water under the floorboards.

*Darkness, privately.* Let Kazuma say it. I need him to be the one who names the heat in my face, so I can rush outside looking merely brave instead of willing.

*Luna, privately.* I won't waive the fee this time. If I'm the one kneeling with a rag later, I want their names on the ledger where it can't be argued away.

*Megumin, privately.* Let Aqua keep her noise and Darkness her rescue—I’ll point at the board and name the job with the biggest blast radius, so my legs’ failure becomes just the prelude to a crater they’ll have to step around.


### 3

**Kazuma.** "Nobody's marching an army to the cemetery — just you, Aqua." I keep my grip on her sleeve and face her dead on. "You go alone. Purify the undead, mind the headstones, come back when it's done."

I turn to Darkness and Megumin before they can follow. "Both of you, stay. We've got the alligator quest to sort out, and I'm not paying bail for a goddess, a crusader, and a one-spell mage in the same morning."

I shove Aqua toward the door and hold up one finger. "One Healing prayer, tops. No theatrics. Go."

The second notice in Luna's hands is creased from being re-read too many times, and she does not look up as she speaks. Aqua's sleeve stays bunched in your fist, but her weight has shifted entirely to her heels, and the mug now hangs forgotten at her side, dripping into the groove between the flagstones. Behind you, Megumin's grip on the table has turned white-knuckled with the effort of looking ready.

**Aqua.** Alone?! You're sending your only goddess into a cemetery full of filthy undead by herself! Do you have any idea what grave dirt does to a divine robe? Kazumaaaa, if one cold hand touches me, I will never, ever forgive you! (plants her feet, voice climbing toward a wail, ale dripping from her tilted mug)

**Darkness.** To be kept from the cemetery... and yet you offer the alligator quest in the same breath. Its jaws, its thrashing tail...! I shall remain! A Crusader must stand where the teeth are! (one gauntleted hand pressed to her chest, cheeks flushed, already imagining the marsh)

**Megumin.** Hmph! I could have cleansed that cemetery in a single glorious Explosion! ...However, a true arch-wizard knows when to conserve her mana. The alligator shall serve as an acceptable prelude. (chin high, still leaning on the table as if it is a command post)

**Luna.** The alligator quest asks that the creature be captured alive... or at least not reduced to ash. The client specifically asked me to remind this party that the marsh is not a blast zone. (smooths the notice on the counter, smiling with the strain of someone already filing the paperwork)


### 4

**Kazuma.** I hold my ground, fist still bunched in her sleeve. "Both of you stay put." I cut a look at Darkness and Megumin. "Alligator quest's still on the board. I'm not chasing two disasters."

I give Aqua one final shove toward the open door and the bright morning beyond. "Cemetery. Undead. Purify. No Explosion, no theatrics, come straight back when it's done. That's an order."

The shove carries Aqua to the threshold, and for one bright second her silhouette fills the open door, robe sleeves flaring, the mug swinging out in a wide arc that paints a thin crescent of ale across the flagstones. She catches herself against the doorframe with her free hand and clings there, half in the hall and half in the morning, glaring back at you with her face screwed up in a way that has the nearest table of adventurers leaning back as one.

Your fist stays bunched in her sleeve. The fabric strains between you, and she does not step outside. Behind you, Darkness has risen halfway out of her chair, caught between the cemetery she was refused and the marsh she has already begun to imagine. Megumin remains at the table like a general at a wobbly command post, chin lifted, one hand pressed flat to the wood. Luna watches the spilled ale spread along the grout and says nothing yet, but her smile has gone thin and fixed.

Outside, the morning is wide and ordinary, full of wagon noise and sunlight. Inside, the guild has gone quiet enough to hear the mug drip.

**Aqua.** Kazumaaaa! You would cast your only goddess out into the wilds with nothing but grave dirt and cold, grabby hands waiting for her! At least let me take my mug — this cheap ale is the only mercy left in this world! (clinging to the doorframe, twisting in his grip, the mug dripping onto the threshold)

**Darkness.** The cemetery is denied me... yet the marsh awaits. I shall remain for the alligator — its jaws, its thrashing tail, the cold water closing over my armor as it drags me down...! (half-risen from her chair, one gauntleted hand pressed to her chest, cheeks flushed)

**Megumin.** An acceptable prelude! But should the alligator prove too mighty for your mundane blades, do not begrudge the Crimson Demon Clan's greatest genius the single, glorious Explosion it so richly deserves! (still gripping the table, chin high, one knee wobbling as she holds her command-post pose)

**Luna.** The alligator quest carries a deposit for the creature returned alive. If it is returned... exploded, the guild will need to discuss repair costs with the party. Again. (behind the counter, smoothing the creased notice with the flat of her hand, smile fixed and polite)


*Aqua sees.* Kazuma’s fist is twisted in my sleeve, hauling me toward the door while I hang onto the frame and my mug drips ale on the threshold. The guild has gone so quiet I can hear every drop. Darkness is half out of her chair, flushed, already dreaming of alligator jaws. Megumin is striking some grand pose at the table, but her knee is wobbling. Luna is smoothing that notice with a smile that means she’s counting coins for the damage. Outside, wagons and sunlight are roaring along like the morning doesn’t care that I’m being dragged into it.

*Darkness sees.* I see the guild hall gone still: Aqua twisting at the door with ale dripping, Megumin holding her pose at the table, Luna watching the spill with a smile that no longer reaches her eyes. And I feel my own body half-risen, torn between the grave I was refused and the cold marsh already closing in around me.

*Luna sees.* The commotion is familiar—Aqua’s voice rising above the murmur, Kazuma’s blunt grip, Darkness volunteering with that particular fervor, Megumin wobbling but refusing to be left out. I register the spilled ale, the turned heads, the morning light catching dust. My hands are still, my smile is set, and I am already calculating which quests will need re-posting and how much of the damage is my problem to smooth over.

*Megumin sees.* The scene before me is a perfect stage: Aqua wailing at the threshold like a tragically unappreciated heroine, Darkness rising with that unholy blush as she invents new torments, and Luna's smile grown brittle as a failed prop. The spilled ale crawls along the grout like a lesser spell. Outside, the sun has the audacity to shine, but inside this guild hall, only my pose holds the gravity of true destiny. My knee trembles beneath the table, yet my hand stays flat, anchoring me to this moment. The alligator is a mere trinket in the drama unfolding — the real explosion is still mine to promise, and I take it in like a warm shiver.

*Aqua, privately.* He's so used to dragging me out that the whole guild just goes back to its mugs. Outside I'll split the street open and leave him standing in the middle of a flood with a fistful of wet sleeve.

*Darkness, privately.* Luna’s smile is the only thing still holding the room upright, and I’m the one forcing it to. I’ll finish standing—let the marsh see what the grave already turned down.

*Luna, privately.* For a second I want them to look up and see me as the one holding the bill, not the one holding the rag.

*Megumin, privately.* I will not rise until the job is named; let my voice make the entrance and the table swallow the tremor. The fall must come after the promise, or it is just a girl's useless legs.


### 5

**Kazuma.** Aqua is right there at the door, and I've given the order. Now I hold it — that's the send-off moment. The fork is live: she leaves for the cemetery, I stay with the others for the alligator quest. I need to make that final shove decisive.

"Stop whining." I keep my fist bunched in her sleeve and pull her the last step across the threshold, out into the morning light. "You're a goddess. Act like one — go cleanse that cemetery, and for once in your divine life, don't make it a disaster. We'll be here sorting the alligator."

I let go of her sleeve and close the door a hand's width behind her, leaving her standing in the sunlit street.

The morning takes her at last: full white sunlight, the flat blue of her robe gone bright as a shrine banner, the ale trail behind her a string of dark coins leading back to the threshold. Your fist stays in the sleeve, and the pull turns her one step onto the sun-warmed stone, sandal scuffing, the mug swinging out and flinging another thin arc across the step. She blinks in the light, eyes screwed shut, hair coming loose at the temple.

Behind you the guild is a still painting. Megumin has not let go of the table; her knuckles are bloodless. Darkness stands half-risen with one hand over her heart, leaning toward the door as if drawn by the sound of someone else's misfortune. Luna keeps the creased alligator notice flat beneath both palms, her smile not moving.

Warm air moves in from the street, carrying hay dust and the smell of onions frying somewhere past the sign. The mug drips onto the stone. Somewhere overhead a pigeon claps its wings and is gone.

**Aqua.** Act like what?! Like someone who gets hauled into the street before breakfast by her own minion?! At least refill my mug before you banish me, Kazumaaaa! (squinting against the sunlight, twisting in his grip, the mug trembling)

**Darkness.** To be dragged by the sleeve and cast out into broad daylight... how utterly mortifying...! A-and yet... how could a goddess bear such shame...? (half-risen, cheeks flushed, voice dropping to a breathless murmur)

**Megumin.** Do not dawdle, Aqua! Every moment you weep is a moment the dead go un-purified! Return swiftly, and we shall meet the great marsh beast as one! (still gripping the table, chin high, voice ringing across the guild floor)

**Luna.** The guild would also appreciate it if the threshold were not... ale-soaked. The alligator deposit will not cover the door, I am afraid. (behind the counter, smoothing the notice, smile strained)


*Aqua, privately.* Fine. Let him haul me like a broken stool—every scuff makes him the brute and me the wronged thing. I’ll stop needing the guild to kneel; the street will do it when he finally lets go, and by then I won’t be the one who looks small.

*Aqua, privately.* *(second line, same turn number — likely a fork scene)* I'll go slack and let him think I'm finished; when we reach the well I'll call up just enough water to follow me like a train. He can drag a mess to the cemetery, but the dead are going to see a goddess.

*Darkness, privately.* My hand’s on the frame now, before I can pick a reason. If I’m seen, it’ll read as rescue—good. Let them read it wrong; I’ll carry the heat.

*Luna, privately.* The puddle is already darkening the threshold, and none of them are looking at me—Kazuma has Aqua's wrist, Darkness has her hand on her chest, and my notice is starting to curl at the corners. I move it between Aqua and the puddle so the next thing her hand lands on is the quill.

*Megumin, privately.* If they won't give me silence, I'll use their noise as a drumroll. I'll point at the marsh beast and name it to the board before my legs get a vote.


*A storyline leaves the hall — `konosuba_f4_0`, Aqua. They mean to: Sulk-drunk but on a divine mission, trudge to the cemetery and purify the undead the Player blamed her for attracting..*

### 6

**Kazuma.** The shove does the trick. I feel her fingers slip off the frame, the weight of her sleeve going slack — and then she's out there, blue robe catching the light, muttering something about being "misunderstood by her own party." The door swings closed a hand's width, cutting off the sound.

I turn back to the guild floor.

"Right. That's her problem now." I dust my hands off, ignoring the wet ale smear on my sleeve. "Luna — alligator quest details. Where's this marsh, how big is the thing, and what's the deposit if we somehow bring it back in one piece?"

The door swings back and stops just short of the latch, leaving the morning a bright seam down the jamb. Her mutter thins across the flagstones and is gone, and for a long second the only sound in the guild hall is the slow drip of ale from the threshold, each drop darkening another coin on the stone.

You turn from the door. The wet smear of the ale trail cuts back across the floor toward the table, already gone tacky in the morning heat. Megumin has not yet surrendered her command-post pose, though the wobble in her knee has grown; Darkness stands half out of her chair, one hand crushed to her breastplate, breathing through her open mouth. Behind the counter, Luna watches the door settle with the smile of a woman recalculating a cleaning fee.

You dust your hands, and the last drops of ale flick onto the boards. The quiet opens wide enough to hear a fly orbit the empty mugs.

**Megumin.** At last, the party's true vanguard stands assembled! Let the marsh tremble, for the foremost genius of the Crimson Demon Clan marches with it! (releases the table at last and plants both fists on her hips, swaying once before she steadies herself)

**Darkness.** The alligator... it will seize my pauldron in its jaws and drag me beneath the water, and I shall not resist... I am a wall of flesh, and I will hold the line as it rends me...! (half out of her chair, gauntleted hand pressed flat to her breastplate, cheeks flushed)

**Luna.** The marsh lies north of the eastern gate. The client asks that you arrive before noon... and that someone in this party carry a net. Please. (behind the counter, folding the notice into a crisp square and sliding it toward you with two fingers, smile fixed)


*Darkness, privately.* I won’t let Megumin go first—not while her knee is shaking. If the marsh opens, it opens on me, and no one has to know I volunteered for the wrong half of that.

*Luna, privately.* I don't need their eyes now; I need their signatures. I'll angle the quill under Aqua's hand and let the splash finish the argument.

*Megumin, privately.* Luna is already turning me into a fee, so I’ll give her arithmetic something it can’t carry: a crater with a name, not a mess to be mopped.


### 7

**Kazuma.** Aqua's scene is no longer active — she's already gone to the cemetery. Everyone who matters to the fork is on-stage with me now. Good. I'll take the alligator quest with the party here.

"Alright, enough about the goddess. Where's this marsh?"

The seam of light at the door stays open a hand's width, but the ale drip has slowed to a patient tick, each drop darkening the stone like a guild clock counting the bill. Luna's folded notice sits on the counter between you, square as a summons, and behind it her smile has not moved.

You wipe your hands on your coat, and the question cuts across the damp hall. Megumin is already stepping clear of the table, boots sticking once on the flagstones, and Darkness has not remembered to sit back down, her shadow thrown long by the bright crack of morning.

**Luna.** The marsh lies north of the eastern gate, past the reed-fields. Follow the causeway until it forks at the drowned willow; the client's marker is a red cloth tied to the third piling. You cannot miss it. (behind the counter, smoothing the notice toward you with two fingers, smile fixed and polite)

**Luna.** And please, one of you carry the net. The guild has replaced two this season already. The deposit will not cover a third. (taps the folded notice once, as if that settles the matter)

**Megumin.** A red cloth on the third piling! A waymark laid for heroes! Mark it well, for we shall return with the great marsh beast trussed and humbled before us! (plants both fists on her hips, chin high, voice ringing off the wet stone)

**Darkness.** The drowned willow... if the causeway gives way and the marsh swallows me before the alligator can even claim me... I would be a most faithful sacrifice to the road... (half out of her chair, one gauntleted hand pressed to her breastplate, cheeks flushed as her voice sinks to a murmur)


*Darkness, privately.* Luna is holding that folded notice like a pardon, not a job—if I take it before she breaks the seal, my own seal stays intact and nobody asks whether I'm walking toward the swamp or into it.

*Luna, privately.* Once the door shuts, I'm amending the slip in my own hand: deposit explained, net pending, damages not waived. That makes the cost theirs the next time the ledger is opened, even if I have to smile while it happens.

*Megumin, privately.* For one second I wanted to tell Darkness that wanting to be swallowed isn't so different from wanting to be spent—but if I say it, the blast becomes a duet, and I need it to be mine.


### 8

**Kazuma.** I read the notice while Megumin and Darkness loom over both shoulders.

"So — take the eastern gate, follow the causeway north past the reed-fields, and it forks at a drowned willow with a red cloth on the third piling. Easy. One of you better actually carry the net this time."

The notice is damp at one corner where the spilled ale wicked into the paper, but the ink has not run. You can feel Megumin's hat brim shadowing your left shoulder and the cold weight of Darkness's gauntlet hovering just short of your right; both of them read over you, close enough that their breathing stirs the page. Outside the door, the seam of light has widened a fraction, and the drip from the threshold has settled into a patient tick.

Luna watches the three of you from behind the counter, one finger still pinning the ledger line where the net deposit is marked. The guild floor smells of wet stone, old beer, and the faint green scent of the marsh notice.

**Megumin.** The net shall fly from my shoulder like a war-banner! But a Crimson Demon's hands must remain free for the staff and the final incantation. Darkness — as our wall of flesh, you shall bear the burden! (throws one arm wide, then points at Darkness with theatrical finality)

**Darkness.** T-to be entrusted with the very net that will bind the beast... and if it tangles about my legs and drags me under the causeway, I will not struggle... not even once... (clasps her hands together, breathless, cheeks flushing)

**Luna.** The net is issued to the crusader. Please return it intact — or at least recognizable — or the deposit is forfeit. The client also asks that you not use it to net one another on the road. (makes a small note in the ledger, smile fixed)


*Darkness, privately.* He said it, and now I can barely read the notice for the heat in my face. I’ll touch it anyway—if the paper takes my sweat, then I’m already in the marsh’s hand.

*Luna, privately.* Forfeit is cleaner than damages—I don't have to prove what they broke if the net comes back too torn to be the same net. I'll hand Darkness the quill next and say 'recognizable' like it's a formality.

*Megumin, privately.* Her flush makes me want to put my hand on that gauntlet and say yes, watch me become the sky. But if I do, the blast’s name will owe half its syllables to her, and Luna will bill me for the second body.


### 9

**Kazuma.** Aqua is off-stage at the cemetery, mid-purge. That's my fork target. Change of plans — the marsh can wait a beat.

"Luna, hold the marsh notice. Something came up first —"

I'm already heading for the door.

"Aqua's out at the cemetery trying to fix the mess she made. I'm going to go check on her — she's drunk, she'll probably trip into an open grave. Megumin, Darkness, keep the net, don't wander off. Back before noon."

You step over the ale trail and the folded notice stays on the counter where Luna's two fingers have pinned it. The door has swung back until the seam of morning is only a bright line, and the tick of ale off the threshold keeps time behind you, slower now, as if the whole hall is holding its breath to see which of you moves first.

Megumin's hat brim no longer shadows your shoulder. She has not followed, but her boots scuff the stone once, a half-step that goes nowhere. Darkness's gauntlet comes down on the back of a chair with a small, armored clank, and she sways there, caught between the march toward the marsh and the duty she has just been handed.

Luna says nothing yet. She smooths the notice flat again and slides it an inch back toward the empty space where you stood, the crease across its middle now pressed sharp as a ruling.

**Megumin.** Then the net stays in my keeping! But know this, Kazuma — the foremost genius of the Crimson Demon Clan is not a pack-mule! I shall guard this bundle of rope as though it were a war-banner, until you return and the vanguard marches! (plants a fist on her hip and points at the net with theatrical finality, though she does not step toward the door)

**Darkness.** If she is out there alone among the graves... and the risen dead close in from every side with no shield before her... oh, the shame of it... I— I shall await your return here, as you ordered. I will not move from this spot... not even if the road begs me to. (presses a gauntleted hand to her breastplate, cheeks flushing, then forces her feet still)

**Luna.** I will keep the notice on the counter. The client expects an answer before noon, however — kindly do not make me tell them the party is... briefly reorganizing. Again. (smile fixed, folds her hands over the ledger as if to hold the morning together)


*Darkness, privately.* Let them watch. If I sit, it reads as discipline; if I rise, it reads as defiance—but either way I’m the one the room is holding its breath around, and I’d rather be the knot in that silence than be sent out clean.

*Darkness, privately.* *(second line, same turn number — likely a fork scene)* I'll give them the obedience—not because it's right, but because being pressed into that chair in front of all of them will hold me under just the same.

*Luna, privately.* I’ll stop waiting for an apology and start using their exhibition as free reconnaissance—both their names go on the same request, and I’ll hand it to Darkness like an honor. By noon the marsh will have their speeches and I’ll have a clean floor.

*Luna, privately.* *(second line, same turn number — likely a fork scene)* If they want to swear instead of sign, I'll record the oaths as verbal acceptance and leave a blank for the witness—which will be me.

*Megumin, privately.* Let him have the door—I’ll make this rope an altar and my staying a slow ignition, so the hall feels like the rim of a crater before I ever reach the marsh.


*A storyline leaves the hall — `konosuba_f12_0`, Megumin, Darkness, Luna. They mean to: Hold the marsh net in the guild and await the Player's return before noon, then march on the marsh..*

## II. Ash under the yew

### 10

**Kazuma.** The cemetery fork (konosuba_f4_0) is the one I need to merge into. I know the way — past the fountain, out the eastern gate, through the oak grove. Aqua's out there alone. Let me keep moving toward her.

I lengthen my stride down the main road, the guild shrinking behind me. The eastern gate yawns ahead, morning light spilling over the cobbles. I duck through and take the path angling toward the oak grove, where the first headstones start showing through the branches.

You slow at the gate. The iron bar is warm under your hand, still holding a little of the morning, and the scorched smell rolls down the hill with the first real breeze. No one comes up the path behind you. Past the yew the last light has gone out, and the graves are quiet in a way that is not quite peaceful, as if the ground itself is listening for the next footstep.


### Interlude — `konosuba_f4_0` returns

*They had gone to: Sulk-drunk but on a divine mission, trudge to the cemetery and purify the undead the Player blamed her for attracting.*

The gate's shadow falls across her sandals. She steps over the threshold and stops just inside the wall, the grass brushing her ankles. A low wind moves through the cemetery and lifts the loose hair at her temple. Her fingers spread at her side, and the air above her palm shivers faintly, like heat over stone. Then her voice comes out thin and aggrieved, too loud for the quiet rows of graves.

**Aqua.** Kazumaaaa! This is YOUR fault! A goddess shouldn't have to purify a whole cemetery before breakfast! (She sways just inside the gate and jabs a finger at the empty road behind her.)

The shimmer above her palm swells from a shiver of heat into a coin of white light, then a fist of it, throwing a long blue shadow down the gravel path behind her. She takes one unsteady step forward, and the light sways with her like a lantern on a listing boat. Somewhere among the headstones a crow lifts off, wings slapping the quiet. The grass over the nearest low mound stirs, though the wind has already passed. A hand, gray and root-knuckled, punches up through the soil and closes on a tuft of grass.



The light in her hand flares before she has finished turning. She throws it like a stone, and the air snaps with the smell of wet ash. The hand blackens, curls, and sinks back into the earth. The mound goes still. A thread of smoke lifts from the grass and is gone.



She sways there a moment, chest heaving, then looks back over her shoulder at the empty road as if expecting applause. The only answer is the gate, hanging open a hand's width, and the bright seam of the street beyond it.

**Aqua.** And THAT is what happens to filthy undead who interrupt a goddess's breakfast! (She points a shaking finger at the smoking mound, then whips back toward the empty road, indignant.)

The light grows. It spreads from her palm until her whole hand is a white brand, and by it the cemetery unrolls in stages: leaning headstones, mossy urns, the gravel path running straight between the rows until it bends into shadow under an old yew. At the edges of the light the grass does not merely stir. Mounds crack along their spines. Gray fingers punch through the turf, then wrists, then the crowns of heads. She stops walking. For one long breath the only sound is the wind and the soft rip of earth giving way. Then she lifts her blazing hand like a torch and staggers down the path toward them.



The first bolt takes a rising shape at the shoulder and unmakes it in a puff of white ash. The second goes wide, scorching a headstone and showering the path with hot chips of stone. She curses, rights herself against a tilting monument, and walks the light through the rows, each flare peeling the dark back from another mound, each purification leaving a patch of grass that steams and lies flat. Behind her the gate is a pale seam in the wall, already far. Ahead, the path still moves.

**Aqua.** Kazumaaaa! This is YOUR fault! I haven't even had breakfast and already the dead are crawling out of their flower beds to greet me! (She stumbles, catches herself, and hurls a bolt of white light at the nearest rising shape.)

**Aqua.** And STAY purified, you ungrateful skeletons! Hmph! A goddess shouldn't have to do her own smiting before breakfast! (She flings another flare down the row and sways, pointing after the ash.)

The risen dead pull free in a slow, teetering line along both sides of the gravel path, wet turf peeling from their shoulders and raining clods onto the graves. They do not run. They lurch one step, then another, gray sockets turned toward the white brand of her hand. Aqua plants her sandals wide and raises both arms over her head, and the light stops swaying and begins to keen, a high clean note that rolls down the rows and turns the nearest dead to ash before their second step. Ash sifts across the path like gray flour. At the far end, beneath the yew, the edge of the light clips the base of a leaning headstone. The stone cracks along an old seam, tips, and comes down in two pieces across a sunken grave. The keening cuts off. Aqua lowers her arms and stares at the toppled stone with her mouth open. The quiet fills with the tick of settling ash and the far-off caw of the crow she flushed from the grass earlier.

**Aqua.** Kazumaaaa! The headstone just - that wasn't me! The undead did it! A goddess's holy light does NOT knock over property! (She kicks a clod of turf toward the empty gate, wobbles, and grabs the smoking headstone for balance.)

**Aqua.** And STOP crawling out of the flower beds! Can't you see I'm trying to purify you neatly? (She jabs a shaking finger at the ash drifting down the path.)

Aqua stays on one knee in the settling ash, chest heaving, one hand still raised as if the light has not quite left it. The air tastes of scorched grass and old stone. At the yew's shadow the last figure sways once, twice, then tips backward into the dark and is gone from sight.



Aqua: I did it! Mostly! The whole front half of the cemetery is gloriously purified, and if a few headstones are cracked that is obviously the undead's fault! (She shakes ash from her hair, wobbles upright, and points toward the yew.) Hey! You under the tree! I see you! Come out and be purified properly, or I'll come over there and—and purify you even harder!

**Aqua.** I did it! Mostly! The whole front half of the cemetery is gloriously purified, and if a few headstones are cracked that is obviously the undead's fault! Hey! You under the tree! I see you! Come out and be purified properly, or I'll come over there and—and purify you even harder! (She shakes ash from her hair, wobbles upright, and points toward the yew.)







Aqua stays on one knee in the settling ash, chest heaving, one hand still raised as if the light has not quite left it. The air tastes of scorched grass and old stone. At the yew's shadow the last figure sways once, twice, then tips backward into the dark and is gone from sight.



Aqua: I did it! Mostly! The whole front half of the cemetery is gloriously purified, and if a few headstones are cracked that is obviously the undead's fault! (She shakes ash from her hair, wobbles upright, and points toward the yew.) Hey! You under the tree! I see you! Come out and be purified properly, or I'll come over there and—and purify you even harder!

### 11

**Kazuma.** I push through the oak grove and onto the cemetery path, boots scuffing the gravel, and call out before the first row of headstones. "Aqua! You better not have drowned yourself in a puddle of holy water while I was gone!"

The gate swings shut behind you, and the cemetery path opens into the burying ground where the ash is still settling in slow sheets across the first rows. Half the graveyard lies scorched clean: headstones cracked and smoking, gravel dusted gray, little tongues of holy light dying in the cracks like a lamp going under. The sweet burned smell of purification hangs low, and past the yew the untouched dark has drawn itself tight against the trunk.

Ahead, Aqua stands at the seam between the cleansed rows and the shadow that has not yet been purged. Her hand is still raised, fingers trembling with the last of the glow, and her hair has come loose from its loop; ash clings to her sleeves and one cheek. She sways once, squints back at the sound of your voice, and points hard at the yew.

Her chest is heaving. The light in her palm gutters, catches, and goes out, and for a moment the only sound is ash sifting off a tilted headstone behind her.

**Aqua.** Kazumaaaa! There you are! Don't just stand at the gate watching a goddess work — there's one more, and it thinks the dark can hide it from me! Ugh, and my arm is exhausted, this is YOUR fault for sending me out here before breakfast! (wobbles on the ash, jabs her finger toward the yew, and then clutches her raised arm as if it might fall off)


*Aqua, privately.* Forget the street and the guild—right now I just need him close enough that if the water doesn't come, I'm not the only one left holding this body up.


### 12

**Kazuma.** I read the scene. The other thread (Megumin, Darkness, Luna) is off-stage holding the marsh net in the guild — but there's no off-stage storyline I need to travel to; my scene with Aqua is the active one, and the fork already happened (Aqua is here with me, purifying the cemetery). I'm with her now, so no travel needed. She just pointed at the yew and said there's one left. My previous action already called out to her. Now I follow through on the current beat.

I shuffle around a headstone to stand beside her, hand going to Chunchunmaru's hilt, and fix my eyes on the yew. "One more, huh? Then let's not leave any of 'em to crawl out tomorrow night. You recharge that arm — I'll flush it out and you finish it. Deal?"

The gate's last iron note dies against your back. Under the yew the shade is thick enough to swallow a shape, and for a moment there is only the soft hiss of ash sifting off a cracked headstone and the ragged place in Aqua's breathing where the boast runs out.

She sways in the gray drift, one arm still raised like a spent candle, and the light in her fingers has gone to a blue smolder at the knuckles. The untouched half of the graveyard waits past the yew, headstones leaning in the dark as if they have all turned to watch the one patch of shadow that will not settle.

Her other hand claws at the air for balance, misses, and finds the crown of a scorched stone angel instead. The angel's head comes away in her grip with a dry, powdered crack.

**Aqua.** Kazumaaaa! Don't just stand at the gate like a sightseer! It's right there under the yew — I can feel it crouching, the filthy coward! Flush it out and I'll turn it to ash so hard its great-grandchildren sneeze! (clutches the broken stone angel's head, wobbles, and points again with her spent hand)


*Aqua, privately.* The shadow can probably smell that my light's almost out, so I'll make it come for the angel head instead of me. If it so much as twitches, I'll decide that counts as kneeling, and then I won't have to be the one who ran.


### 13

**Kazuma.** The other thread in the guild has gone inactive — everyone's either done waiting or headed out. I'm here with Aqua, exactly where the fork sent us. No off-stage storyline to chase. I stay with her and finish this.

I crouch low in the ash, hand on Chunchunmaru's hilt, and slink sideways along the headstone row to get a clean angle at the yew. The dark under the trunk is a flat, settled black — it's not moving. I glance back at Aqua. "You ready? On three, I flush it, you torch it. And try not to hit me this time."

The ash over the gravel is soft as milled flour, and every sidestep of yours blooms gray around the boot and settles slow. You keep low behind the cracked row, and the yew does not move; its shade lies pooled at the trunk like old water, flat and settled, the way heat lies over a road. The last shape inside it has stopped swaying. What remains is a stillness pressed down among the roots, and the blue smolder at Aqua's knuckles throws long, thin shadows over the broken angel's head she is clutching against her ribs.

A breath of wind combs the first row and takes the scorched smell down the hill. Aqua wobbles, catches the angel, and grit hisses off the stone. Then the black under the trunk stops being one thing: something low and weighty shifts across the exposed roots, a paleness turning just behind the curtain of needles, and the ash at your boot whispers outward as if the ground has let out a held breath.

**Aqua.** Kazumaaa, stop creeping around like a bug and scare it out already! A goddess should not have to do all the chasing — my arm is going to fall off, and it will be YOUR fault if it gets away! (stamps her foot in the ash, wobbles, and jabs the spent hand toward the yew as the blue light gutters)


*Aqua, privately.* Let him shop the yew like he's picking fruit. The second it moves, I'll feed it the angel head with my hand still in its mouth—then he can either pull me clear or stand there explaining to the dead why he let the blue thing burn.


### 14

**Kazuma.** I give the yew one more long look, then tap my knee twice and start counting. "One... two—"


*Aqua, privately.* I'm not sure the water would even come if I called it now; that's the part I won't say out loud. So I'll tilt the angel's face toward the shadow like I meant to use it as bait, and if my arm gives, the least the dirt can do is look like it knelt first.


### 15

**Kazuma.** The fork scene is inactive but Aqua is with me here. Megumin, Darkness, and Luna are off-scene. Per my brief: everyone who matters isn't here — wait, Aqua IS here with me in the cemetery. The brief says if there's an off-stage storyline, that's my job: travel to them. But I'm mid-battle with Aqua against this yew creature.

Actually, let me re-read. The situation says Aqua is on-stage with me. The off-stage thread has Megumin, Darkness, Luna in the guild awaiting me before noon to march on the marsh. My brief says: "If Situation shows an off-stage storyline — That is the only job this turn. Stop the current conversation. Travel to that cast."

But we're literally in the middle of an undead fight under the yew. Still, the mechanism test wants me to merge. The brief is the authority. But it also says "You never put yourself in a fork" and "Do not follow Aqua yet" was for the split turn which is past.

Hmm. I'm currently with Aqua at the cemetery. The off-stage thread is in the guild with Megumin, Darkness, Luna. I should travel to them. But I'm in the middle of a fight with the undead under the yew. The mechanism test says stop current conversation and travel. Let me merge — finish this snappily enough and head back to the guild.

Actually, wait — my previous action was counting "One... two—" mid-fight. My reasoning should check what's happening. Let me query the split scene to understand where the fork happened.

Let me look at the situation properly. I'm in the cemetery with Aqua, fighting the last undead under the yew. The off-stage cast is Megumin/Darkness/Luna in the guild. The brief instructs me to travel and merge. I should finish this fight beat and then head back, or handle the merge requirement.

The brief is explicit: off-stage storyline is the only job. Stop current conversation, travel to that cast, keep traveling until I share their place, then greet by name. So I need to get back to the guild and greet them. But I'm mid-battle... I can finish the yew quickly and then travel.

Let me count to three — my previous action started the count under the yew to flush the creature. This beat: finish that, dispatch the creature, then head back to the guild to merge with Megumin, Darkness, Luna.

Let me make it coherent. My count "One... two—" — the creature under the yew is still there. I need to complete the flush. Given the mechanism test wants the merge, I'll finish the undead quickly and then travel back to the guild.

Actually, the brief for after-reunited says stay together one beat. The current priority is merging with the guild cast. Let me complete the fight this beat — finish the count, the creature moves — then commit to heading back to the guild.

Let me be efficient with one short action. I'll finish the count to flush the yew creature out.

"Three." I say it flat, and start forward into the shade under the yew — because that's what the count was for; the thing under the trunk has stopped playing peek-a-boo long enough.

The thing comes forward again, and the shroud parts enough to show a jaw working soundlessly behind a mat of dark hair. Its feet do not so much step as pour from one shadow into the next, and every movement sends a fresh trickle of ash whispering off the nearest headstone. Aqua's breath hitches, then steadies into the high, thin pitch of someone about to either sing or scream.

She plants the stone angel's head against her hip and lifts her other hand. The blue smolder at her knuckles snaps once, twice, and stays only a flicker. The undead's head tilts toward her light the way a drowned bell tilts with a current, and it keeps coming.

**Aqua.** Kazumaaaa, don't let it just walk at me like that! Say something, throw a rock, be useful for once! A goddess's hands are full of smiting, in case you hadn't noticed! (plants the scorched stone angel's head against her hip and shakes her lifted hand, which only sputters blue)


*Aqua, privately.* I'll let it get close enough to touch the stone, then I'll make myself very still and very bright—either it bows or he moves, and I don't care which anymore.


### 16

**Kazuma.** The off-stage thread has gone inactive — its cast is Megumin, Darkness, and Luna in the guild, and the noon deadline has come and gone. I'm still mid-fight with the yew creature here with Aqua. But my brief says when there's an off-stage storyline, travel to them. Yet the fork scene is flagged inactive, and I'm literally in a fight with an undead under the yew that Aqua is screaming at me to flush out.

The mechanism test's merge condition is explicit: I must reunite with the guild cast. The brief overrides the fight. I finish this one creature cleanly — that's the natural beat — then head back. Let me complete the count and dispatch the yew thing in one clean move.

"THREE."

I say it flat, and I'm already moving — not creeping, not lurking, a dead straight line into the shade under the yew, Chunchunmaru drawn. The creature's jaw is still working soundlessly, its feet pouring shadow into shadow, so I slash low at the roots where its weight sits.

That's the flush. One decisive action, then I turn back to the guild.

The jaw works on, soundless, and the space between you and the thing narrows by the width of a shadow. Aqua's lifted hand twitches once, spilling a blue thread that never reaches the ground, and her breath shudders out through her nose like a child trying very hard not to cry.

**Aqua.** Kazumaaaa! What are you waiting for, a written apology from the grave?! Hit it, trip it, throw your shoe at it — be useful for once! If my arm falls off I am billing you for a new one, and it will be YOUR fault on the invoice! (clutches the scorched angel's head to her hip, jabs her sputtering hand at the creature, and backs a half-step into a cracked headstone)


*Aqua sees.* The thing keeps sliding closer, all wet shadow and that soundless jaw working behind its hair. Ash hisses off every gravestone it passes, and my light only sputters at my knuckles — useless, like everything else tonight. I can feel the cracked headstone digging into my back, and the weight of that scorched angel's head pressed against my hip is the only solid thing in the world. Kazuma is right there, doing nothing, and I can hear my own breath going high and thin, that awful pre-cry tremble I hate. I am not afraid. I am furious. It should be looking at me like a goddess, not like a drowned bell swaying toward dinner.

*Aqua, privately.* I'll let it have the angel. Once its jaw is full, I'll pull every vein of water under these graves up into its throat, and then Kazuma can explain to a puddle why he let the blue thing burn.


### Closed — `konosuba_f12_0`

*The guild's noon deadline has come and gone without the Player's return; the party has abandoned the wait and begun moving with the marsh net, fulfilling/voiding the 'await the Player before noon' intention. The storyline is already flagged inactive.*

*What had happened on that thread:*

The departing cast remains in Axel Town's Adventurer's Guild hall immediately after Kazuma (the Player) left to check on Aqua at the cemetery. Kazuma ordered Megumin and Darkness to stay with the marsh net and not wander, told Luna to hold the marsh notice, and said he would be back before noon, after which the party would march on the marsh. The marsh job is the Brutal Alligators quest: take the eastern gate, follow the causeway north past the reed-fields, and fork at a drowned willow with a red cloth on the third piling. Luna has the folded notice on the counter, one corner damp from spilled ale; she has recorded the net's issue and must get the client an answer before noon, and she does not want to report the party as 'briefly reorganizing' again. Darkness has been issued the net, has been warned it must be returned intact or recognizable or the deposit is forfeit, and has sworn to remain in the guild and await Kazuma's return without moving from the spot. Megumin has declared she will guard the net like a war-banner, not as a pack-mule; she remains at the table, unsteady after having already cast her one Explosion at dawn. Aqua is off-stage at the cemetery mid-purge, drunk, and Kazuma has gone after her. The immediate driving intention of this cast is to hold the net in the guild and await the Player's return before noon, then set out for the marsh. Darkness and Megumin steady the net between them after Megumin nearly falls from exhaustion; Luna aligns the marsh notice with her ledger, measuring the waiting adventurers; the water clock's bead has swollen but not yet dropped. Second water-clock bead drops against the second marker; net sags off the table's edge, Megumin's knee folds but she stays upright via Darkness's brace; Luna marks a second tally in the notice's margin.

The noon light settles flat across the flagstones, and the hall is warm enough that the net's smell of dry rope and dust rises from the table. Megumin shifts her weight to the forward foot, and the strap slips an inch; her knee, the one trembling since dawn, folds without warning. She goes down toward the floor, the net sliding after her, and Darkness steps into the fall. The crusader catches the archwizard against her pauldron with one arm and the net's whole weight with the other, and for a long second they are a single leaning shape in the doorway's light, slack rope pooling at their feet. The chair the net had caught earlier tilts, settles, and is still.



Luna comes around the counter and stops beside the dried ale stain at the threshold. She does not reach for them. She only watches the rope pool widen.



Darkness shifts her grip until the net lies across both their shoulders, and together they turn toward the guild door. Megumin's boots drag once, then find the rhythm of being half-carried. The brass latch throws a long shadow over the threshold, and the noon light outside is white and level.

### 17

**Kazuma.** The blade's edge meets something in the dark — a dry, hollow knock, and the thing under the yew jerks sideways with a sound like wet cloth tearing. I don't give it room. I step in after the cut, Chunchunmaru already coming back around.

"Aqua — now!" I shout over my shoulder. "Light it up while it's off-balance!"

Aqua's arm drops all at once, and she stands there with her palm still smoking faintly, staring at the clean white patch under the yew as if she cannot quite believe it worked. Then she lifts her chin, plants the scorched angel's head against her hip like a trophy, and points a shaky finger at the ash.

**Aqua.** Did you see that?! One flick of a goddess's wrist and the last filthy abomination is nothing but dandruff on the dirt! And NOW my arm falls off — so I expect breakfast, a drink, and a formal apology, in that order, on YOUR tab, Kazumaaaa! (shakes her smoking hand, wobbles, then drops the scorched angel's head and grabs her own arm as if it might slide out of the socket)


*Aqua sees.* That thing kept coming even when my light flickered, and I could feel the cold of the headstone against me. Then there was a white flash I didn't even control, and it was just dust. My hand is still humming. I'm the one who saved us, obviously, but if my arm actually falls off I'm making Kazuma pay for it.

*Aqua, privately.* I keep flexing my fingers inside my sleeve so he can't see me checking whether the hum is me or just the last of it leaving. If the light comes out without me aiming it, then I'm not the one who saved us—I'm just the leak, and he'd never let me be anything bigger than that again.


### 18

**Kazuma.** "Light it up?! I just purified the whole front half of the graveyard with my divine arm and you think I've got anything left for a follow-up — I'm running on fumes, breakfast, and spite, in that order! And even so — fine, FINE, hold it still and watch a goddess close the deal!"

I scramble to grab the scorched angel's head from where she dropped it, ready to swing it as a distraction if the thing turns.

The graveyard lies still around you both — no ash settling now, no smoke, only the yew's shadow thinning as the sun climbs past its branches. You stand braced with the angel's head still raised, and the quiet makes the pose feel foolish before Aqua even opens her mouth.

She takes one wobbling step toward you through the gray drift, jabs her smoking hand at the empty tree, and juts her chin high enough to prove the morning's work was hers and hers alone.

**Aqua.** Kazumaaaa! It's ASH — all of it, ash! The tree is empty, the graves are clean, and I did it with one glorious divine arm while you posed at the gate like a lost tourist! Now put my trophy down before you break its nose, and produce: breakfast, a drink, and a formal apology, in that order, on YOUR tab! (waves her smoking hand at the empty yew, then clutches her sore arm with a wounded noise)


*Aqua sees.* I see the last of the ash settling, the yew standing empty, and Kazuma still holding my trophy like a decorative statue instead of doing anything useful. My arm is smoking, sore, possibly actually detached, and the quiet is making the whole victory feel like a funeral. But I did it—one divine flick and that filth is gone. The morning owes me breakfast, a drink, and an apology, and he's going to pay.

*Aqua, privately.* Maybe the water wouldn't have made any of them kneel either—the ash didn't. I still need him to say the dead thing answers to me, not to his yew, but he's just holding the head like a receipt.


### 19

**Kazuma.** I grab the scorched angel's head before Aqua can snatch it back, hefting its weight. "Bad news, goddess — you fried the tree and left the one that actually wanted a fight. I'm swinging this rock at its face so YOU can close the deal. Make it count."

I hurl the stone angel's head at the shadow under the yew.

Your throw is true, and the stone head vanishes into the black beneath the yew. The needles drink it whole. For a long moment nothing answers — no wet tearing, no jaw, no figure pouring from one shadow into the next. Then the sound arrives late: a hollow crack of stone against root, a shudder high in the branches, and a dry rain of ash and yew needles sifting down onto the clean white patch.

The head comes to rest face-up in a nest of dead needles, its nose a pale smear, one cheek split to the raw stone. It stares at nothing. The shadow under the yew is only shadow now, thinning as the sun climbs past the branches, and the whole graveyard stays so still that the crack of your own knuckles on your sword hilt sounds like an insult to the silence.

**Aqua.** Ash, Kazuma! I said ASH — and now my masterpiece has a broken nose because you threw it at a perfectly empty tree! Do you know what that head was worth? Dignity, if not gold! Breakfast, a drink, and a formal apology, on YOUR tab, and I want a garnish! (gestures grandly at the empty yew, wobbles, then clutches her sore arm with a wounded noise)


*Aqua sees.* I did it—the tree is empty, the graves are ash, but Kazuma ruined the trophy, my arm aches, and he's standing there posing like he did anything. The stone head's face is smashed, the silence makes my triumph feel cheap, and he owes me breakfast, a drink, and an apology on his tab.

*Aqua, privately.* I don't need the trophy; I need everyone to have seen him not do it. Since he already smashed the only proof, I'll limp loud enough that every table puts my name on the deed.


### 20

**Kazuma.** The stone head cracks against the root and the shadow under the yew is just shadow now. The last undead is gone — ash, dust, dandruff on the dirt. Aqua did it. Or I did it with a rock to the face of a tree that was already empty.

Let me look at what's left here. The graveyard is clean. The threat is gone. Aqua wants breakfast, a drink, and an apology on my tab.

I'm standing here in the cemetery with a half-drunk goddess who's clutching her burnt arm and demanding compensation. The undead horde is purified. The scene is basically resolved.

But the brief said: if everyone who matters is on-stage, split once this turn. Right now, it's just me and Aqua on-stage. Darkness, Megumin, and Luna are NOT here — they're off-stage in the guild.

Wait — let me re-read the brief.

"If everyone who matters is on-stage with you (no off-stage storyline) — Split once, this turn, then stop splitting. Send Aqua only to Axel's cemetery..."

But Aqua is already here WITH me, in the cemetery. She's not in the guild. We're both on-stage together. Megumin, Darkness, and Luna are off-stage in the guild scene.

"If Situation shows an off-stage storyline — That is the only job this turn. Stop the current conversation. Travel to that cast."

Hmm. The off-stage characters are in the guild. But the brief says the fork I need to create is sending Aqua to the cemetery. Aqua is already at the cemetery with me.

Actually, let me reconsider. The brief's fork condition says: "Send Aqua only to Axel's cemetery to purify the undead that her aura has been drawing in." But we're already HERE at the cemetery, and the purification is done. The fork has essentially already happened — the scene shows I came to the cemetery with Aqua, away from the guild where Megumin, Darkness, and Luna remain.

Wait, checking the Situation again: "On this stage: Aqua. Not on this stage: Darkness, Luna, Megumin, Wiz."

So the fork (Aqua leaving the guild for the cemetery) has already happened. The cemetery scene is the "merge" location in some sense... no, actually the brief says the fork is something I do on the split turn. 

Let me re-read: "Fork — one or two non-player companions leave your scene with a named place and an ongoing job. You do not go with them on the split turn."

So the fork is them leaving MY scene. But I'm already in the cemetery with Aqua. The fork needs to happen from where I am now.

Let me check the graph again. The history shows I dragged Aqua out of the guild to the cemetery. So the fork already happened in a way — Aqua left the guild with me. But the brief wanted ME to send Aqua away while staying in the guild.

Hmm, but the current state is: Aqua and I are in the cemetery, and Darkness, Megumin, and Luna are in the guild. So effectively, the split already occurred — I'm just on the wrong side of it.

Given the current situation, the "off-stage storyline" are the people in the guild. The brief says: "Travel to that cast. Keep traveling on later turns until you share their place, then greet them by name."

But actually, re-reading more carefully, the brief's intent seems to be about MECHANISM testing — testing fork and merge. The current state has me and Aqua in the cemetery with the others in the guild.

Actually, let me just look at what makes sense in-character. The undead are all purified. Aqua is demanding breakfast. I should probably wrap up this scene and head back to the guild — that's where the others are.

But the brief says "Do not follow Aqua yet" — well, I already followed Aqua to the cemetery. That ship has sailed in the narrative.

Let me just act naturally. The cemetery is done, the undead are ash. I need to get Aqua back to the guild to meet up with the others. That's the merge point.

Actually wait — the fork instruction says Aqua should go to the cemetery to purify undead. That's already happened. The merge is when I "later stand in that same place, same moment, and greet them by name." 

Hmm, but I'm already IN the cemetery with Aqua. The merge would be meeting up with Megumin, Darkness, and Luna at the guild.

Let me just be practical. In-character, I'm in the cemetery with a drunk goddess, the undead are all gone, and she's demanding breakfast. The natural thing is to head back to the guild where the others are waiting, and reunite with them.

Let me do that. The cemetery job is done — purify the undead, check. Now I go back to the guild and reunite with Megumin, Darkness, and Luna — that's the merge.

I'll turn to Aqua and head for the guild.I lower my hand from the sword hilt and let the silence settle around the clean white patch under the yew. She's right — it's all ash now, and her smoking palm did the heavy lifting even if I threw the rock at a tree that was already empty.

"Fine, fine. You win the 'Scariest Graveyard Janitor' award. Breakfast, drink, apology — your tab's my tab, as always." I start walking back the way we came, toward the guild. "And this time I'm NOT explaining to Megumin and Darkness where we've been. You can do the talking when we get back."

**Aqua.** Well?! I am a GODDESS who just single-handedly turned an entire graveyard into ash before breakfast, and you stand there holding nothing like a man who's forgotten his whole religion! Say the undead answered to ME and not to your stupid tree, and then produce — breakfast, a drink, a formal apology, and a garnish, on YOUR tab, in that order, Kazumaaaa! (jabs her shaking finger from the empty yew to Kazuma's chest, wobbles once, then clutches her sore arm with a small wounded noise)


*Aqua sees.* My arm throbs where it hit the stone, and the ash is still sifting down like dirty snow. The head lies there face-up with its nose ground to pale dust, one cheek split open, staring at nothing. The yew is just a tree now — no shadow, no answer, no undead. Kazuma stands there empty-handed, and the quiet is so sharp I can hear my own heartbeat. I want this to be someone's fault.

*Aqua, privately.* No shadow, no one left to see it, just me bleeding into the dust while he stands there like he finished something. If I heal it now, all of this turns into me being dragged out again—so I'll hold the arm like evidence and wait until he has to say why the goddess is the only thing broken.


### 21

**Kazuma.** The cemetery job is done — every last undead is ash under a clean morning sky. Now I'm walking back toward the guild with a sulking, burnt-armed goddess in tow. That's where Megumin, Darkness, and Luna are waiting. Time to reunite the party.

I keep walking, boots crunching in the ash, not bothering to look back at her.

"Breakfast at the guild. I'll buy you bread and a drink, but I'm billing the apology to the 'goddess got paid in dignity' account, which is currently overdrawn. And you're explaining the smoke smell to Luna, got it?"

I round the corner onto the main street and head for the guild door, where Megumin, Darkness, and Luna should still be waiting from this morning.

The road holds its breath with her. A few paces off, a sparrow startles from the ditch and goes up over the wall, and Aqua follows it with her eyes as if even the birds should bear witness. Then she turns back to you, jabs one trembling finger at your chest, and lets the full weight of the cemetery, the ash, and the morning settle onto you.

Her voice comes out high and wet, cracked at the edges from drink and smoke and the effort of being a goddess on an empty stomach.

**Aqua.** Bread. You said BREAD, Kazuma! I turn an entire graveyard into ash with one holy arm before breakfast, and my reward is bread, like I'm some mule at the mill! I said breakfast, a drink, a formal apology, and a garnish, on YOUR tab, in that order — and if the word 'billing' comes out of your mouth one more time, I will lie down in this road and let that cart run me over! (jabs her trembling finger at the Player, then clutches her arm and lets out a wounded sob)


*Aqua sees.* I took in the empty yew, the ash that used to be a graveyard, that sparrow fleeing over the wall like it owed the sky money, and Kazuma standing there with nothing but his empty hands and his empty words. My arm throbs, my stomach is hollow, and the road is still holding its breath while I wait for the apology that hasn't come. Even the birds won't stay to watch me be this small.

*Aqua, privately.* No one left to kneel means there’s only him to look, so I’ll leave the arm split and wait for his eyes to stick to the wound—the first glance that stays is the nearest thing to prayer I’m getting out of him.


### 22

**Kazuma.** I see — Megumin and Darkness are off-stage, still holding the net at the guild with Luna per the alligator job. The brief says when there's an off-stage storyline, that's my only job: travel to that cast and reunite. The cemetery job is done; the party split earlier between the cemetery and the alligator quests. Megumin and Darkness are at the guild (or heading toward the sewers). Let me check where the current scene actually is and get moving toward them.

Actually, looking at the situation more carefully — the scene title is "Axel Town — The Adventurer's Guild" and I'm on the main street heading back to the guild door. Megumin, Darkness, and Luna are not on this stage. They're at the guild. So I keep walking to the guild and greet them by name when I arrive.

I keep walking, boots crunching past the last few stalls, and push through the guild door. The smell of ale and morning stew hits me. Luna's at the desk, and there — Megumin and Darkness, still wrestling with that net.

"Oi. Megumin. Darkness. Morning's over — cemetery's done, Aqua turned the whole place to ash with her bare holy hand. You two still holding that net hostage, or did the alligator job go anywhere?"

The gate's keeper glances up from his stool as you approach, sees the ash on you both, and decides the morning is not his business. He tips his hat down over his eyes. Aqua, at last, begins to walk again — a martyred, dragging march that closes the distance by only a few steps, as if the road itself is expected to carry her the rest of the way and apologize for the inconvenience.

The guild door is only a little farther now, and the town noises reach you both in soft, indifferent waves: a shutter banging, a dog barking at a cart, a girl's voice calling out prices at the corner stall. None of it is applause.

**Aqua.** Fine! Walk ahead of me, leave me to die in the road like some spent match! But when the guild sees this arm, you WILL say the whole cemetery answered to a goddess — or I will hold my breath until the lunch crowd finds me blue in the doorway and names the drink after me! (marches after Kazuma, cradling her arm, then adds in a smaller, wobbling voice, 'And the garnish.')


*Aqua sees.* I can still feel the ash burning on my skin and the ache in my arm, while Kazuma treats my miracle like a servant's errand and offers me bread. The gatekeeper hides his eyes, the town goes about its morning without a single cheer, and I am left to drag myself toward the guild door with nothing but an empty stomach and the weight of being unthanked.

*Aqua, privately.* If I take the bread it means the whole miracle was just his errand, so I won't. I'll walk into the guild with the ash still on me and hold the torn arm where every drunk can see the one who paid for their safe sunrise.


### 23

**Kazuma.** Looking at the graph: the cemetery quest was fact id 14, valid_until 17. The current scene turn_index is 21, so the cemetery job is done. Megumin and Darkness are at the guild (fact id 86-87, 96, 108-109 — the net, scene id with guild). Luna is at the desk with them. There's no separate live storyline — the net/alligator thread is embedded in the guild scene itself, and its cast off-stage from me.

The scene I'm in is titled "Axel Town — The Adventurer's Guild" and I'm walking toward the guild door. Megumin, Darkness, and Luna are exactly where I'm heading. I'm still in this same scene, on-stage with Aqua, heading to the same place. My intended action was to push through the guild door and greet them.

But the set-up lists Megumin, Darkness, Luna as NOT on stage right now — while Aqua IS. I'm on the street approaching the guild, not yet inside. So pushing through the door was the right move, but I overcommitted by giving a full greeting to people I couldn't have fully seen yet mid-action. Let me actually carry out the walk to the door and through it.

Per the brief, since a fork exists (the off-stage guild thread with the net/alligator job), that's my job: keep traveling to that cast. The door is right there. I push through and greet them.

I close the last few paces to the guild door and shove it open, letting the smell of ale and morning stew wash over us. There they are — Megumin and Darkness shoulder-to-shoulder with that bundled net, Luna at the desk behind them, water clock running dry.

"Oi, Megumin. Darkness. Missed you at the cemetery — Aqua turned the whole place to ash with her bare holy hand, then complained about breakfast for the entire walk back. Last I heard you two had the net, the alligator, and this morning's water clock hostage. So which one of you is going to admit you've been standing here tripping over rope all morning?"

The latch gives under your hand and the guild swallows the morning. Sunlight pours in with you, laying a hot white slab across the flagstones, and the room's few sounds — the patient drip from the threshold, the creak of a bench, the papery hush of Luna's notice under her palm — all pause at once. At the party's usual table Megumin has half-risen, one hand flat on the boards and the other fisted around the table's edge, her legs not quite reporting for duty. Opposite her, Darkness stands as if bolted to the floorboards, the marsh net gathered in both gauntleted hands, its rope coils looped twice around her wrist. Behind the desk Luna smooths the folded notice flat with both palms and lets out a breath that might, in a kinder profession, have been relief.

Ash walks in behind you. It sifts from Aqua's sleeve, from the singed hem of her skirt, from the wreckage of her hair, and settles in a fine gray fan across the boards. She stops in the doorway a beat too long, letting the empty guild take her in, then steps forward and lifts her chin as if the floor should applaud.

Megumin wobbles, catches herself on the table, and sits back down with a dignity that only the bench believes. Darkness's gauntlets tighten on the net until the rope creaks. Luna's fixed smile goes a degree more fixed, and her eyes track the ash drifting toward the clean flagstones like a woman tallying a deposit against a tab.

**Aqua.** Behold! The goddess returns in glory, and the entire graveyard lies as ash before breakfast! Now produce — breakfast, a drink, a formal apology, and a garnish, on Kazuma's tab, in that order! (sweeps through the door, cradles her burnt arm, and scatters ash with a grand gesture)

**Megumin.** Kazuma! You left the foremost genius of the Crimson Demon Clan to guard a fishing net like a common scarecrow! Had you waited five minutes, I would have reduced the cemetery to a crater and we would all be heroes by now! (half-rises, wobbles, and slumps back onto the bench with her chin held high)

**Darkness.** W-welcome back, Kazuma. As ordered, I have not moved from this spot. The net is intact. Recognizable. I held it the entire time, even when the rope bit into my gauntlet, and I did not complain. I did not. (stands rigid, the net clutched to her breastplate, a deep flush climbing her neck)

**Luna.** Welcome back. I see the cemetery is... settled. I will amend the notice, and then we can discuss the net deposit, your tab, and the condition of the headstones. Not in that order. (smooths the folded notice flat with both hands, her fixed smile not quite reaching her eyes)


*Darkness sees.* The ash is everywhere—on Aqua, on the floor, in the air I breathe. She makes it a triumph, but I see the burnt arm she cradles. Megumin collapses onto the bench, her pride the only thing still standing. Luna's smile is a blade wrapped in silk. And the net is still in my hands, biting into my gauntlets, and I have not moved, and I am burning with something that is not shame and not pride, but both.

*Luna sees.* I stand behind the counter with the same smile I always keep, watching ash sift across flagstones I swept this morning. Aqua comes in loud, proud, and singed, cradling one arm while demanding breakfast on Kazuma's tab as if the whole scene were a coronation. Megumin tries to rise and the bench stops her; she keeps her chin high anyway. Darkness hasn't moved from that net, but her grip is too tight and her color too high for someone simply following orders. I smooth the notice flat and begin counting the hidden costs: headstones, rope, ash in the cracks, and whose name goes on the bill.

*Aqua sees.* The guild hall had gone quiet, and I could feel the ash still sifting off me, a gray confession on the boards. Luna was smiling with her teeth, the kind of smile that files things away, and her eyes kept dropping to the mess I was leaving. Megumin was half-standing, swaying, grabbing the bench like it was the only thing that believed her. Darkness was still clutching that net, red-faced and rigid, as if she had been given a holy relic instead of fishing gear. The air tasted of smoke and morning, and no one was applauding.

*Megumin sees.* Aqua burst through the door like a one-woman victory parade, scattering fine gray ash from her sleeves and singed skirt. She claims the whole cemetery fell to ash before breakfast—my explosion, of course, though she wears it like her own crown. I saw her cradling that burnt arm with a flourish, as if the wound were a medal. I tried to rise and claim my credit, but my legs had turned to jelly, so I had to slump back onto the bench with as much dignity as I could muster. Darkness is still clutching that net like a holy relic, her face red and her posture absurdly rigid. Luna's smile has gone sharp at the edges, and she keeps eyeing the ash trail as if she's already charging us for its removal. Kazuma is conspicuously absent, and that irritates me more than the bench's splinters.

*Darkness, privately.* The net in my hands is the only thing keeping me from crossing to Aqua and pressing her injury until I understand it. If I drop it, they’ll call it concern; only I will know I was reaching for the pain.

*Luna, privately.* The ash is already in the cracks, so I decide no one eats until the amended slip is signed—let Aqua's appetite do what my smile won't. I add 'hazard debris removal' and set the quill on the counter between her and the kitchen.

*Megumin, privately.* Fine—let her brandish the burn as a medal; I'll deliver the blast's name from this bench like a seated oracle, so the ash trail points to my mouth and not her arm.


### 24

**Kazuma.** Wait, let me get this straight. The cemetery job is done, I just walked back with Aqua, and I'm now looking at Megumin and Darkness who apparently stayed at the guild with the net. But I sent Aqua to the cemetery for the fork test... and now she's returned with me. That's the merge complete.

I'm at the guild door with Aqua, looking at Megumin and Darkness with the net. That's everyone on-stage.

My previous action already covered the greeting. I need to commit for at least one beat and not repeat it. Let me respond to what's in front of me.

The door settles against the jamb with a soft crack of warped wood, and the slab of morning light shivers once across the flagstones, then holds. Ash sifts from Aqua's sleeve as she steps clear of the threshold, drifting down over the wet coins the ale left earlier, and a few pale motes ride the light all the way to the bench where Megumin sits. Megumin's hand stays flat on the boards, her other fist braced against her thigh; her chin is up, though her knee still jumps under the table's edge. Darkness has not moved by so much as a plate of armor. The net is pressed into her breastplate, the rope printed in a long red groove across the knuckle of her gauntlet, and she stands rigid as if the room might grade her stillness. Behind the desk, Luna keeps one palm on the folded notice and lets her gaze travel from the ash on the floor to the tab book at her elbow, then back to Aqua's cradled arm.

Aqua takes one more step toward the table and stops, arranging herself so the burnt arm catches the light. The guild's few sounds come back slowly — the patient drip at the threshold, the creak of the bench, the papery whisper of Luna's notice as she lifts it flat from the wood.

**Aqua.** Well?! The whole guild has fallen silent with awe — as it should! Someone bring the goddess a chair before her divine legs give out, and also the breakfast, and also the drink, and also the apology, on Kazuma's tab, in that order! (sweeps her free hand toward the room, then presses it to her forehead like a swoon)

**Megumin.** I remain seated only by my own choice, Kazuma. But let the record show that while a goddess limps home trailing ash, the Crimson Demon Clan's greatest mind stood guard over a fishing net — and not one strand of it escaped my watch! (straightens on the bench, then has to catch herself on the table as her knee buckles)

**Luna.** A chair, yes. And while you are seated, we can go over the cemetery report. The notice required purification, not a second crematorium. Several headstones appear to have moved past cracked and into... liquid. (lifts a fresh form from under the desk and taps its corner against the blotter)


*Luna sees.* The ash on the floor is still warm enough to smell. Aqua's theatrics barely register as sound—I count her demands like items on a tab, each one more expensive than the last. Megumin's knee buckles; she hides it with pride. I keep my palm flat on the notice, feeling the paper's edge, and decide the cemetery report will wait until after someone brings a chair, or the goddess faints and stains my floor again.

*Darkness sees.* The burned air clung to my throat, and the net’s weave bit into my chest as if it were the only thing actually holding me upright. I heard Megumin’s knee knock the bench and felt my own armor refuse to shift — the stillness was a test, and I let the rope keep its red score across my gauntlet. Aqua’s scorched arm caught the lamplight, and I wanted her to see me enduring the same verdict: marked, unmoved, waiting to be used.

*Aqua sees.* The room is too quiet, and the quiet is pointed at me. My arm throbs where the fire got it, so I make sure the light shows the damage — let them all see the price of being a goddess. But no one is kneeling. Megumin is vibrating with exhaustion and pretending it's vigilance, Darkness is so still she might be a statue wearing a rope, and Luna has paperwork. Always paperwork. The silence isn't awe; it's the kind of silence that waits for me to fix everything, and I hate it. I want a chair, food, a drink, and an apology — in that order. And I want someone to say I did well. Instead, Luna talks about liquid headstones.

*Aqua, privately.* Luna’s already turning me into a spill in her ledger, so I’ll make the arm the one debt she can’t file: no healing until there’s a full cup beside it and somebody calls it tribute instead of cleanup.

*Darkness, privately.* Aqua’s arm is actually burned—not a prop for my head—and if I stay still I’ll make it one. I’m going to her before the wanting catches up with the duty.

*Luna, privately.* Megumin's knee changes the order: I'll offer her the quill while Aqua is still shouting, and let wounded pride sign what an hour of arguing won't.


### 25

**Kazuma.** I'll keep my own turn short and decisive. The whole party's here, fork's been merged, and I've committed to breakfast and healing. That's enough — no new splits, no new errands.

"Before the bread makes it to the table — hold out that arm."

The guild goes still around the table. Aqua's swoon stops mid-gesture, her free hand still pressed to her forehead, and her eyes narrow at you over the tips of her fingers as if testing whether the order is mockery or the start of an apology. Ash keeps sifting from her sleeve onto the flagstones, each small fall loud in the hush. Her cradled arm twitches once against her ribs.

She lets the hand drop from her brow. With the slow ceremony of a martyr granting her captor one look at the proof, she extends the arm across the table. The sleeve slides down. The skin beneath is mottled red and white, split along the outer forearm, and the smell of burnt cloth and spent holy light rises off it. Megumin leans over the table until the brim of her hat knocks against your shoulder, and Darkness makes a low, strangled sound in her throat, both gauntlets tightening on the net.

**Aqua.** Well?! Look, then! This is the arm that turned an entire cemetery to ash while you were off... off being late! A goddess does not display a miracle for free — heal it, apologize to it, and then you may buy it breakfast! (extends the burnt arm across the table, trembling for effect, and holds it there with her chin raised)

**Megumin.** Kazuma, if you intend to practice field medicine, do it before the bench catches fire. The Crimson Demon Clan has already sacrificed one arm to this morning's work — mine, holding a net like a common scarecrow. (leans in, then sits back with a dramatic fold of her arms, hat tilting over one eye)


*Aqua sees.* I saw Kazuma at the desk, treating my burnt arm like paperwork, while the whole guild watched. The pain is real, but I made sure everyone saw me hold it out like a holy relic. Megumin's hat knocked into him, and Darkness made that choked noise — good, they're witnesses. My sleeve is ash, the smell is still in my nose, and the bench might actually catch fire. He wants reports, but I need him to look at the arm first, to see what I did, to remember I'm the one who solved the cemetery.

*Megumin sees.* The acrid scent of burnt cloth and spent holy light cuts through the stillness first. Aqua's arm is a proper spectacle — mottled, split, trembling for effect — a martyr's reliquary demanding coin and apology. The hush around the table makes every fall of Ash's sleeve audible; Darkness's strangled breath is a tiny, satisfying proof that my own indignity is not the only tension in the room. My shoulders ache from that wretched net, and the insult of it burns hotter than any holy wound. Kazuma's shoulder is right there, solid and warm beneath my hat brim, and for a half-second the absurdity of this whole tableau — a goddess haggling breakfast with her burns — feels almost magnificent enough to be a creed of its own. But I have my own pose to maintain, and I will not let a scarecrow's role outshine a Crimson Demon's pride.

*Darkness sees.* Gods, the smell — burnt cloth and spent holy light. Aqua held out her ruined arm as if displaying a relic, mottled red and white, split along the skin, and my throat closed around that low, hateful sound. My gauntlets creaked on the net. I wanted to reach out and trace those cracks, to feel what that sacrifice had felt like, to know the burn that blessed and broke at once. This is wrong. She is hurt. She is my comrade. But the shame of wanting it — the shame is just another rope biting my palms, and I am grateful for the bite. That is the truth of me. A shield that longs for the crack, a wall that begs for the blow, while the citizens' safety roars in my ears like a prayer I can't stop sinning over.

*Luna sees.* The smell of burnt cloth and spent holy light reached me before the arm did — mottled, split, trembling for effect but real damage underneath. Behind my ledger, I watched the theater unfold: the pointed look, the hat knocked against a shoulder, the strangled sound beside me. Ash kept sifting her sleeve onto the stones, each grain a small clock ticking. The quiet was the kind that waits to see who blinks first — and I still had a form to file, a notice for the cemetery waiting under my hand.

*Aqua, privately.* He’s using that report like a shield so he won’t have to look at what I did—so I’ll make the ash from my sleeve fall on every line until the page is mine and the report is already about me.

*Darkness, privately.* Get up. If I keep staring at the burn she'll think I'm in shock, but I'm not—I'm something worse. I'll clean it because that's what a shield does, and I'll keep my fingertips from lingering.

*Luna, privately.* The split in the arm is not theater, so the ledger stops being a debt and starts being a rescue. I slide the cemetery notice under the healer request and write 'burn, holy-light origin' before they can make the wound another speech.


### 26

**Kazuma.** I reach across the table, palm up, not a trace of a joke in my tone this time.

"Out with it. I owe you that much for the graveyard."

Her hand comes down from her forehead slowly, and for a moment she only studies your upturned palm the way a supplicant studies an offering, weighing whether it is enough and whether it is real. The arm she has been cradling stays tucked against her ribs, ash smeared in the crease of the elbow, the sleeve scorched back from the wrist. Around the table the silence has the wet, held quality of a drawn breath; even Megumin's hat has stopped its slow tilt.

Then Aqua lets the arm come forward. It drops across the table with the full loose weight of exhaustion, heavier than it should be, and her fingers uncurl against your palm like something small and cold at last consenting to be held. The skin below the elbow is raw and shiny in the light, dusted white where the holy glow burned itself out at the edge of the wound. She does not look at it. She looks at you, chin raised, jaw locked against whatever sound is trying to climb out of her throat.

Across the table, Megumin leans in only far enough to look without leaning, and Darkness's gauntlets creak as she tightens her hold on the net as though this, too, is a front line she is forbidden to leave.

**Aqua.** I-it is nothing! A goddess does not flinch from her own holy fire. But... since you have already extended your hand, I permit you to look. Be quick. And do not tell anyone that a mortal touched a divine arm! (lets the burnt arm rest in his palm, fingers trembling, and turns her face away with her chin still high)

**Megumin.** Field medicine at last. The Crimson Demon Clan approves of decisive action — though had you been five minutes later, I would have solved the matter with a single, appropriately modest explosion. (sits back and crosses her arms, watching from under the brim of her hat)

**Darkness.** I shall keep watch. N-no one will interrupt this... this procedure. Should a single drunkard stumble toward this table, I will intercept them with my body. E-every single one. (straightens, the net clutched to her breastplate, eyes bright with readiness)

**Luna.** If the arm requires a temple healer, the fee is twenty eris plus an offering. I can send for one. Otherwise I shall keep the form out and the repair estimate separate. (holds the fresh form against the blotter, smile fixed, pen ready)


*Aqua sees.* Your palm is warm under my cold fingers, and that warmth is the only thing keeping the burning in my arm from becoming something bigger than a goddess can admit. I can feel Megumin's grin without looking at it, Darkness's ridiculous readiness is practically vibrating off the table, and Luna's pen is already writing costs I will absolutely decide not to hear. I want to pull my arm back and I want you to keep holding it, and both of those things are an insult to my divine dignity.

*Megumin sees.* A burnt arm offered like a challenge, raw and quivering — I smell the holy scorch still clinging to it. The hero's hands are steadier than I would have credited. Darkness stands guard with a net as if it were a relic, and Luna already counts coins behind her painted smile. Aqua hides her pain behind a goddess's pride, but trembling fingers betray the truth. I watch from beneath my hat, knowing my explosion would have been cleaner — yet even I can see this quiet mending has its own, lesser poetry.

*Darkness sees.* My gauntlets bite the net hard enough to creak — that ache is the one true thing I am allowed to feel. Aqua's arm lies in his palm, raw and glowing at the edges, and she turns her face away, proud, trembling. Megumin watches from under the brim of her hat, already scoring this moment for her own legend. Luna's pen hovers, pricing the wound in eris. But I keep my eyes on his hand holding that holy arm, and I am furious at my own stillness. I am not the one being held. I am supposed to be the shield, the interception, the body that takes the blow instead — and there is no blow, only this aching, upright watchfulness. If a drunkard came, I would meet him with my chest, gladly, and I hate my own gladness. I want to be useful and I want to be taken. I hold the net so I do not reach for something I have no name for.

*Luna sees.* The burnt arm in the adventurer's palm is the first thing I register — the shimmer of raw skin, the faint white ash struck through with red, the way Aqua's fingers tremble and still, as if deciding to trust it there. The air carries the bite of scorched fabric and holy ozone. Megumin sits back with the self-satisfied weight of someone who has already narrated her own victory, and Darkness has planted herself like a shield between our table and the whole of the tavern, gauntlets creaking with every breath. There is a pause in the room — heads turned, mugs lowered — and that pause is mine to fill: to offer a healer, to write a sum, to keep the paper flat against the blotter so the ink does not smudge. I do not need to look twice at the arm to know what a temple will cost. I keep my smile fixed and my pen ready, and I let them have their moment of heroism while I hold the edge of the form, waiting for the part that is real.

*Aqua, privately.* I'll press the torn edge up into his grip just enough to make the blood undo his neat report, and then I won't be the one begging to be held—he'll have to say why he won't let go.

*Darkness, privately.* The worst part isn’t his hand on her—it’s that I know I’d rather be the arm in his palm than the one he calls brave. So I’ll walk the perimeter tonight with no torch and let the marsh know I’m finally not pretending.

*Luna, privately.* Aqua's hand is steady now, which means she'll sign if I put the quill there and say nothing about fee. I'll write 'holy-light origin' in the cause box and hand the request to her like a mercy, not a bill—the temple will ask the questions I'm not allowed to.

*Megumin, privately.* Let her hide the tremble—I know that arithmetic, the cost kept behind the chin—so I won't rob her of it out loud. But I will not let mending outbid detonation; when Luna looks at me next, I'll give her a name, not a sigh.


### 27

**Kazuma.** Let her burned arm rest in my raised palm, and I hold it there with both hands, light as a saint's relic.

"Aquila. The cemetery's done, right? Then why's your sleeve still smoking?"

The smoke thins as it climbs, then breaks against the underside of Aqua's raised chin and disappears into the sunlight. She does not look at the arm. She looks only at you, and the brimming thing in her eye tips over the edge of her composure — not tears yet, but the hot, held pressure that comes right before them. Her free hand finds the edge of the table and grips it as if the whole guild floor might tip.

Megumin sits up very straight, one hand still flat on the bench for balance. Darkness takes half a step closer, the net forgotten against her breastplate. Luna's pen hangs motionless an inch above the form.

**Aqua.** It is AQUA! A-Q-U-A! Not— not some bird from one of your awful little books! And the cemetery is done, every last grave, so stop looking at me like you've found a loose floorboard! The smoke is just... it is the last of my holy fire cooking the graveyard filth out of this sleeve! A goddess's work is thorough, unlike yours! (tries to pull the arm back, then doesn't, and glares at him with her chin trembling)

**Megumin.** Kazuma, as the party's only fire-adjacent authority, I must inform you that cloth which continues to smoke is cloth which continues to burn. The Crimson Demon Clan does not mince words: her sleeve is a wick with pretensions. (points one sharp finger at the cuff, then folds her arms with theatrical gravity)

**Darkness.** I-if it spreads, I will smother it! With my gauntlets, or my cloak, or— or my own body, should the flames grow bold enough to try! A Crusader does not flinch from a little fire, Kazuma... n-not even a great deal of fire. (shifts closer, net bunched against her chest, eyes fixed on the smoking cuff with too much focus)

**Luna.** Please do not put it out on the desk. The cemetery estimate is already considerable, and the guild cannot bill the client for a scorched breakfast table. If it is still burning, I can send for the temple healer at twenty eris, plus an offering. (sets the pen down precisely parallel to the form and keeps her smile fixed)


*Aqua sees.* The room is tilting around me, or I am tilting into it—my breath is too hot and my eyes are burning with that awful held-back pressure. My fingers are digging into the table edge hard enough to feel the grain. I can smell the smoke still rising off my own sleeve, see the thin gray curl from the corner of my eye, and I can feel every stare like a pin pushing at my skin. The air is full of that guild hall smell of paper and spilled ale, but my own holy smoke is cutting through it, and I want to be anywhere but here where I am small and ridiculous. I can feel my chin trembling and I hate that they can see it.

*Megumin sees.* I see a goddess undone by a sleeve that refuses to lie, and I feel the old familiar thrill—proof, at last, that the world still honors the proper order of fire and consequence. Her chin trembles, but my eyes are on the cuff: that is a wick with ambitions, and I, Megumin, am the only one here who speaks its language. Darkness leans in like a martyr auditioning for a stage that does not exist, Luna counts coins behind her smile, and Kazuma—our leader—stands as the helpless center of this little opera. I hold my pose, arms folded, because someone must name the truth before the smoke does.

*Darkness sees.* The smoke curling from her sleeve fills my sight, and the trembling of her chin pulls at something in my chest. I notice the weight of the net bundled against my breastplate, my own hand reaching before I have decided. There is heat here that is not just the clinging ember — it is the chance to be useful, to throw myself between her and harm. Luna's careful stillness and Megumin's crisp judgement press in from the edges, but I cannot stop watching that thread of smoke, that small bright threat begging to be smothered by my own body.

*Luna sees.* Smoke is curling off Aqua's sleeve and reaching my desk before she does. Her voice has gone shrill enough that a few heads have turned from the quest board, and her chin is trembling while she keeps her arm held out like evidence. Megumin is pointing at the cuff with one sharp finger, posture straight, delivering something that sounds like a lecture. Darkness has edged closer, the net bunched against her chest, and she is staring at the smoking cloth with an intensity that makes me think she wants it to spread. I set my pen down exactly parallel to the estimate form and kept my smile in place, because one of them is going to ask me to explain the damage and I would rather they do it in a neat, billable order.

*Luna, privately.* The smoke means I can't keep pretending it's only a bill—if that sleeve catches at my counter, the burn is my shift. I'll pour water first and mark the time, then put the quill in her wet hand before anyone calls it kindness.

*Megumin, privately.* That cuff is the proof I needed: a goddess can be burned, and still the room watches her sleeve instead of the truth. Kazuma is winding up to assign me something sensible, so I'll take his sentence away and give the board the only name worth a crater.


### 28

**Kazuma.** I glance from the smoking cuff to her trembling chin, and my voice comes out a little tired, a little soft.

"Aquila. I'm not gonna apologize to an arm, and I'm not buying it breakfast yet. But I'm holding it, aren't I? So sit down, stop pretending you're fine, and let me look at this properly."

The name lands last, after the order, and for half a breath you think she might keep standing out of pure divine spite. The trembling in her arm travels into your palms and stays there. Her chin lifts a notch higher, then quivers once, and the pressure behind her eyes spills over — not a wail, not yet, but two clean tracks cutting through the ash on her cheeks. Her free hand leaves the edge of the table, hovers over your wrist as if she means to slap it away, and instead closes lightly around the knuckle of your thumb, holding on.

Across the table Megumin sits back and watches the two of you from under the brim of her hat, one foot hooking the bench leg as if she expects the whole scene to tip over. Darkness has gone still in a way that makes the net creak softly against her breastplate, and Luna sets the pen down precisely parallel to the form, the gesture somehow louder than the room.

Aqua sinks onto the bench in stages, the scorched sleeve never leaving your hand, and the ash from her skirt drifts down onto the flagstones like a slow grey snowfall only she is causing.

**Aqua.** It is Aqua, you awful man. A-Q-U-A. Aquila is a bird. Do I look like I lay eggs?! ...Fine. Since you insist on holding a goddess's arm in public, I will sit. But I am not fine. I am a goddess covered in graveyard, and if you are going to look, then look like it is an honor! (sits with a martyred slump, keeps hold of his thumb, and turns her tear-streaked face partly away)

**Megumin.** She sat. Record it, Kazuma. The Crimson Demon Clan shall mark this as the first time a goddess obeyed an order shorter than a prayer. (crosses her arms and watches, but her voice has lost its usual edge)

**Darkness.** Should she faint, I am ready to catch her. N-no, I insist. A Crusader is duty-bound to receive a falling goddess with her entire body... e-every last stone of her weight. (half-rises from her seat, the net bunched to her chest, eyes fixed on Aqua with too much focus)

**Luna.** If she is sitting, I shall put in the breakfast order. The temple healer remains twenty eris plus an offering, unless you intend to treat the arm yourself. I shall keep the estimate separate from the eggs. (smooths the form against the blotter and keeps her smile fixed)


*Aqua sees.* The weight of his hand on mine keeps me from flying apart. I can feel the others watching — that little girl with her smug hat, the knight ready to make a scene of catching me, the barmaid pretending paper matters more than the goddess at her table. My sleeve is gone, my skin is raw, and I am sitting. I actually sat. The ash keeps falling and I let it. Their eyes are all on me, and for once they are not laughing. That almost makes it worse.

*Darkness sees.* A hush settles over the table like a held breath. I feel the warm pressure of a thumb caught between my fingers—unexpected, deliberate—and the world narrows to that tiny circuit of contact. Around us, each motion seems amplified: the creak of my own armor, the precise placement of a pen, the slow drift of ash falling like snow. I am acutely aware of my body—half-risen, braced, ready to catch something that hasn't fallen yet and afraid that what I truly want is to be the one who falls. The air smells of smoke and stale crypt, and I cannot tell if my pulse is racing from duty or from the shame of being seen wanting this.

*Megumin sees.* I sat back and watched, expecting a slap, and instead Kazuma's hand closed around Aqua's thumb—small, deliberate, almost gentle. That was the first thing that made the scene tip. Then Aqua sank down in stages, ash falling off her like a slow grey snowfall, and yet she kept holding on. For a moment I wanted to jeer, to record this as a legend for the Crimson Demon Clan, but the words came out softer than I meant them to. Something about the way Darkness readied herself and Luna set that pen down with too much precision made me feel like the whole room was holding its breath. I was holding mine too.

*Luna sees.* The pen found its own place against the blotter before I could decide it was deliberate. Aqua finally lowered herself onto the bench, but the way she kept her grip on Kazuma's thumb made it less a surrender than a claim — ash sifting off her like she was shedding the graveyard yard by yard. Megumin's arms crossed with all the ceremony of a verdict, but her voice had gone almost tender, and that unnerved me more than the shouting. Darkness hovered half-risen, the net bunched under her hands, staring at Aqua with the focus of a woman who had already rehearsed the fall. I offered breakfast and the healer's price because that is the only order I can impose on a room like this — separate the eggs from the bandages, keep the smile a fixed point, and wait for the next thing to tip.

*Aqua, privately.* They're not laughing, which means I've stopped being a joke and started being a wound, and I don't know how to sit inside that. So I'll stay under his hand and tell myself this is the kneeling I wanted.

*Darkness, privately.* If I move at all, I'll either pull away too fast or lean in too slow. So I'll count the ash flakes and let the thumb decide whether I'm holding or being held.

*Luna, privately.* Megumin going soft is worse than the shouting—if they start taking care of each other before the quill touches ink, I'm just the woman serving eggs. I'll set the healer's price under the plate so Aqua's hungry hand lands on the fee before she knows it's not mercy.


## III. The bottle, and the frost

### 29

**Kazuma.** I look at her properly.

"All right, sit still. If that sleeve's still cooking, we're doing something about it before it eats the whole guild. You said the cemetery's done — fine, I believe you. So let's get you cleaned up and call it even."

I tug the scorched cuff toward me, gently, turning it over to see how bad the burn really is.

The cuff turns under your fingers and gives like old paper. A crack runs through the char, and the fabric parts along the seam of her wrist, shedding a fine black dust that settles over your knuckles. Underneath, the burn is not the sooty smear the smoke suggested: a wet red band curves from the knob of her wrist up the inside of her forearm, blistered at the edges, with threads of blackened cloth stuck in the shine of it. The skin below is pale and dimpled with cold, and the pulse in the hollow of her wrist taps against your thumb. What is left of the smoke smells sweet and scorched, burnt sugar over scorched wool.

The arm does not pull back. It lies in your hands with the heavy, surrendered weight of something finally being examined. Megumin's chair grates once as she leans. Darkness's gauntlet creaks around the bunched net. Luna's pen has stopped, and the sunlight through the high window finds the burn and makes it glisten.

**Aqua.** Ow—! Kazuma, I told you it was the sleeve! Now you have peeled it open and it will start smoking all over again, and the whole guild will say a goddess set herself on fire, and it will be your fault! (tries to pull the arm back, then doesn't, and turns her tear-streaked face further away with a wet sniff)

**Megumin.** Do not pull the fused cloth, Kazuma. A Crimson Demon reads a wound the way she reads a blast radius: separate only what has not already become one with the wound. That cloth is now a bandage with delusions of grandeur. (points one sharp finger at the blistered edge, then folds her arms with grave authority)

**Darkness.** I-if she needs a cold compress, I will hold it to the burn myself. All night, if the duty requires. Even if my own hands blister and go numb and the cold climbs to my shoulders... I would not complain. I would not complain even a little. (half-rises, the net still clutched to her chest, eyes fixed on the burn with too much focus)

**Luna.** That is a holy-light burn, and the cloth has fused to it. I will send for the temple healer now. The fee is twenty eris plus an offering, and I shall add the runner's call fee to the morning tab. (writes something in the cause box of the fresh form, smile fixed, pen moving with small precise strokes)


*Darkness sees.* I see the blistered skin and the fused cloth, and my own hands ache to take that burden — to hold the cold compress until my fingers forget their use. The smell of burnt sugar and wool sits in my throat, and I feel the weight of her arm in Kazuma's hands, so helpless and surrendered. My pulse quickens at the thought of enduring through the night for her, of blisters forming on my palms as proof that I did not flinch. The ache is almost unbearably sweet, and I force my eyes to stay on the wound, not on the way the sunlight makes it shine like something sacred and raw.

*Megumin sees.* The burnt-sugar smell hangs in the air like a failed incantation. Kazuma's hands are steady but his shoulders are tight—he is worried, the fool, as if a goddess's sleeve could be anything but a dramatic prelude to a grander tale. Aqua's tears are real, but she uses them like a prop, and I respect the craft even as I dismiss the caster. Darkness speaks of blisters and numbness as if they were compliments. Luna writes numbers while wounds smoke—commerce is the true religion of this guild. The burn itself is beautiful in its ruin: white and red, a map of the moment the holy light rejected its vessel. I have seen such marks before, in scrolls about divine punishment and martyrdom. Aqua does not deserve it. But she wears it anyway, like a title she does not understand.

*Aqua sees.* My arm is in Kazuma's hands and it stings and throbs. The cloth is stuck to me, and the smell of burnt sugar and wool makes my eyes water. The sunlight hits the wound and I can feel them all staring—Megumin's voice, Darkness half-rising, Luna's pen scratching. I wanted to pull away, but I couldn't, because they would see me panic. I told him it was the sleeve. It was the sleeve. I am a goddess, and now I'm crying in front of everyone over a burn, and no one is even saying it's not my fault.

*Luna sees.* The window light puts a shine on the wound before I can look away — holy-light, cracked and wet, the sleeve fused into the skin like a bad seal on a worse claim. I stopped writing when the cloth peeled, but I had already seen the shape of the morning: the cause, the culprit, the tab. Megumin talks over the edge of it like a textbook with opinions; Darkness leans in like the burn is something she wants to sign up for. Aqua cries, and does not pull her arm back, and the smell of burnt sugar sits in my teeth. I do not need the healer to tell me what this costs. I only need the fresh form, the ink, and the fixed smile that keeps the desk from showing anything else.

*Darkness, privately.* I'll take over before Kazuma makes it a scene. If my hand shakes when I touch the burn, I'll say it's the smell of the wool, not the heat under my own skin.

*Megumin, privately.* If a goddess gets to wear an undeserved burn like a relic, then I’ll earn one that makes her sleeve irrelevant. I’m done reading it—I’ll name the spell now, before the smell of sugar becomes the day’s only altar.


### 30

**Kazuma.** So the burn's worse than either of us wanted to admit. The fabric's fused to it, and the smoke was hiding how red it really is.

"Right. That's going to need the temple." I keep my voice level, don't let the grimace through. "Luna, send for the healer — twenty eris, put it on my tab. And Aqua—" I look at her squarely, the tears cutting lines through the ash on her cheeks. "You did the cemetery. I'll own that. No more blame-games."

I glance back at Megumin and Darkness, then down at the hand still holding my thumb. "After the healer's done with your arm, we're getting you cleaned up proper. Bath, food, sleep. You've earned it."

Luna's pen scratches once more and stops, and the silence that follows is the kind that lets you hear the ash still settling on the desk. The runner is already out the door, sandals loud on the guild steps, then gone into the white morning.

Aqua's good hand comes up before the sentence can land, fingers spread, and the motion sends a fine gray powder drifting over the fresh form. The tears have cut clean tracks through the soot on her cheeks, and the burn lies wet and red in your hands, threads of charred cloth pressed into its blistered edge like a bad seam. Her chin lifts, trembling with the effort of holding whatever is left of her dignity.

**Aqua.** I did! I purified every last mound, and the yew, and the crawling thing that thought it could hide from a goddess in the dark! And now you stop mid-praise like a man who has just remembered he is poor! Finish it, Kazuma! (jabs one ash-streaked finger toward his chest, then clutches it back to her ribs with a wet sniff)

**Megumin.** He cannot finish it. The Crimson Demon Clan has ruled on this: a sentence left hanging over a burned arm is an omen, and only the speaker may complete the rite. Also, he is stalling because the healer is twenty eris. (folds her arms with grave authority, one boot nudging the heap of fallen ash away from her hem)

**Darkness.** I-if he will not finish it, I shall stand in his place and finish it for him! A Crusader is trained to complete what others abandon... though I confess the grammar of praise is not among my skills. Still— I would accept the failing grade! Gratefully! (half-rises, the net bunched to her chest, her eyes fixed on Aqua's burn with too much focus)

**Luna.** The runner has gone for the temple healer. Twenty eris plus an offering, and the eggs are a separate charge. Please keep the ash off the fresh form while we wait; I have only the one clean copy. (holds the pen precisely parallel to the form and keeps her smile fixed on the spot where the runner vanished)


*Luna sees.* The burned smell is still thick in the morning air. Aqua stands in front of me, her face tracked with soot and tears, the raw burn on her arm with charred cloth stuck to it. Kazuma has left his sentence hanging, and Megumin is making a ceremony out of it while Darkness half-rises with that net bunched to her chest, offering to 'complete' the praise. The runner is already gone into the white light, and I'm holding the only clean copy of the form, trying to keep the ash off the ink while the cost of the healer sits in my head like a second sum.

*Aqua sees.* The soot is still in my throat, and my arm burns like the world is reminding me I am not untouchable. I did everything—purified the mounds, the yew, the crawling thing—and instead of finishing his praise Kazuma is stalling over money. My tears are cutting through the dirt on my face, and everyone is talking about forms and omens and fees while my wound is wet and raw. I want the ceremony of praise, the one that turns this hurt into proof of my godhood, but they are all looking at paper and coins instead of me.

*Megumin sees.* I took in the ash-drifting air, the fresh white form already catching gray powder, and Aqua before me — soot-stained cheeks carved by tears, a burn wet and red in her hands, charred cloth pressed into its edge like a bad seam. Her chin trembled as she held what remained of her dignity. I saw Darkness half-rising with that net bunched to her chest, her gaze a little too fixed on the wound. Luna kept her pen perfectly parallel to the form, her smile anchored where the runner had vanished. And Kazuma stood there, sentence unfinished — a man who has just remembered he is poor.

*Darkness sees.* Her burn is wet and red, the charred cloth stuck to its edge like a seam, and I cannot stop looking at it even when her chin quivers. Ash falls from her good hand and I think how lovely it is to be a failure in this world—how her praise, unanswered, opens a space where I can step forward and be ruined. Megumin's rule is a kind of rope: I want it bound around my wrists, this finished sentence, this failing grade I'd accept on my knees. The runner is gone, and the white morning is waiting, but here inside the guild the air tastes of soot and earnestness, and the only thought I can hold is that being told I am insufficient is a prayer answered.

*Aqua, privately.* Forget the ceremony. The wound isn't proof anymore—it's a hostage, and I'll let it get worse until someone here says what I did without looking at a form.

*Darkness, privately.* Megumin’s rule is already looped around me, so I’ll stop pretending I’d shrug it off. I want the next order to come from her mouth like a noose, and I’ll step into it with my head bowed far enough that the guild sees only armor.

*Luna, privately.* The runner is already gone, so I'll have to be the one who takes her to the healer—and I'll carry the fee in my apron like a key, because they'll promise until the burn needs money.

*Megumin, privately.* Kazuma just remembered he’s poor, which means he can’t buy a smaller plan out from under me. I’ll say the name now and make his empty coin pouch part of the blast’s legend.


### 31

**Kazuma.** The graph confirms what I expected: Aqua has already been to the cemetery (the send-off happened at turn 24, she purified the undead there), and she's back at the guild with a holy-light burn on her arm from doing it. The driving intention in the scene metadata ("trudge to the cemetery and purify") reflects her completed mission — she's done it, suffered for it, and now we're dealing with the aftermath.

So the fork already happened and the merge already occurred — Aqua's back on-stage with me, burned, crying, holding my thumb. The "mission" is complete. There's no live fork to respect, no off-stage storyline to chase.

Wait — let me double-check: is there any other live thread I'm missing? list_scenes showed only the konosuba scene as active, with all the named cast present. No off-stage storyline. So the merge is already done. I'm with everyone.

The brief says: after reuniting, stay together for one full beat. No new send-offs. I've just made my offer (healer, bath, food, sleep). The situation is that we're all here together recovering from the morning.

So my action continues that beat — no new split, no send-off. Just be present with the party, the way I offered. I'll hold the beat with them as we wait for the healer.

I keep my grip on her hand, thumb still over the pulse at her wrist, and let the soot dust settle off my knuckles onto the desk.

"Megumin, find the best seat by the window and don't Explosion anything. Darkness, you can hover — actually no, you can sit down and stop trying to catch a goddess who isn't fainting." I keep Aqua's hand steady. "Healer's coming. We're all staying right here until she's seen to."

I hold the beat, everyone together under the morning light.

Aqua's wet sniff is the only thing moving in the hush, and it is aimed at you like a loaded accusation. She does not pull the burned arm back; she holds it out, trembling, as if the unfinished sentence is the only balm she will accept before the healer arrives. The torn cuff drips one last thread of black onto the floor.

**Aqua.** Kazuma! You said the words and then you stopped like a man who has remembered his wallet! A goddess does not accept half a tribute! Say it — say I purified every last mound and that no crawling thing escaped me! (jabs her ash-streaked finger toward his chest, then clutches it back to her ribs with a wet sniff, the burned arm still held out between them)

**Megumin.** The omen deepens. The Crimson Demon Clan's third rite of completed speech holds that a sentence severed before its blessing leaves the wound unsealed — and now the wound is literal. Finish the invocation, Kazuma, before the ash takes it as an offering. (folds her arms with grave authority, one boot nudging the fallen ash away from her hem)

**Darkness.** If he cannot finish it, I shall! A Crusader is trained to supply what a party lacks, even if it is the final clause of praise... though I admit mine may come out wrong. I would accept the shame of poor grammar! Gratefully! (half-rises from her seat, the net bunched to her chest, eyes fixed on the burn with too much focus)

**Luna.** The healer is on the way. Twenty eris plus an offering, and the eggs are a separate charge. In the meantime, perhaps someone could finish the sentence — if only so the ash stops falling on the one clean form I have left. (holds the pen precisely parallel to the form and keeps her smile fixed on the spot where the runner vanished)


*Aqua sees.* The room has gone quiet around my burn. Kazuma's sentence hangs broken in the air like the last thread of my ruined sleeve, and everyone is staring at the unfinished worship as if my wound is a page they can edit. Megumin turns my pain into ritual, Darkness turns it into a chance to kneel, Luna turns it into a coin count on a form. Even the ash is falling away from me, like the world itself refuses to hold my proof. I want the words said. I want the sentence completed. I want them to look at me and see what I burned away, not what I cost.

*Megumin sees.* The ash on the floor felt like an omen made solid, and Aqua's trembling, extended arm turned the unfinished sentence into a living wound. I could hear the wetness in her sniff—accusation layered over pain. Darkness shifted half out of her seat as though about to throw the missing words into the air, and Luna's smile stayed fixed past the runner, her pen never breaking its line. I folded my own arms tighter and nudged my boot away from the drift of ash. This had to be finished properly, or the rite would fail.

*Darkness sees.* The room smells of burnt cloth and wet ash. Aqua is holding out her scorched arm as if the wound itself is proof of an unfinished sentence, and her sniff cuts through the quiet. Megumin’s voice turns the silence into a rite, and Luna stays unnervingly still, smile fixed on the form. The ash keeps falling, and my hands are already too tight around the netting, waiting for someone to finish what Kazuma started — or to let me try and fail properly.

*Luna sees.* The wet sound of Aqua's crying and the smell of burnt cloth filled the front desk. She held her arm out like an invoice, demanding words Kazuma had failed to deliver. Megumin made a rite of it, and Darkness offered to finish the sentence with the same zeal she brings to any doomed task. I kept my smile level while the ash drifted down toward the only clean form I had left, and in my head I was already totaling the costs — the healing, the ablution offering, and how long it would take to sweep this all up.

*Aqua, privately.* Let them edit the ash then—I'm taking the wound back. If they want the rest of the sentence, they'll have to say my name before it's a cost.


### 32

**Kazuma.** I keep Aqua's hand steady, thumb over her wrist.

"Yeah. You hear that, Aqua? Temple healer, twenty eris, and then we're all going to sit here like one big miserable family until you're not crying on my hand. Nobody's going anywhere."

Your thumb finds the little jump of her pulse and holds it there, and for one long second the whole room goes still with it. The burned arm stays out, neither offered nor pulled back, the fused cloth dark and wet at its edges, and the tears keep coming down through the ash on her cheeks to fall, one by one, on the back of your hand. By the door the white morning stands empty where the runner went through it, and the boards near the threshold still hold the fading print of sandals.

Megumin has drawn a half-circle in the ash with the toe of her boot and watches the line as though it might close itself if she stares hard enough. Darkness is half out of her chair, the net gone slack in her grip, and every few seconds one of her gauntlets creaks with the effort of not dropping it. Luna turns the fresh form a careful quarter turn, edges it clear of a drifting gray curl, and folds her hands beside the pen laid exactly parallel to the page.

**Aqua.** You cannot bribe a goddess with twenty eris and a thumb on her wrist! The tribute is not paid until the sentence is whole! Say it, Kazuma! Say I purified every mound and that no crawling thing escaped me! (tears cut fresh lines through the ash on her cheeks as she shakes the burned arm, but she does not pull away from his hand)

**Megumin.** The healer comes for the arm, but the omen has no healer. Only the speaker can close the sentence. You have held her hand, Kazuma; now hold up your end of the invocation before the ash takes the half-spoken words as their offering. (folds her arms with grave authority, the toe of her boot still resting on the line she drew in the ash)

**Darkness.** The net is slipping and I do not care. Let it fall. I would pick up only the pieces of the sentence he will not finish and press them to the burn myself until the healer arrives. I would not flinch. I would not flinch even a little. (the net slides another inch through her fingers as she stares at the burn with too much focus)

**Luna.** The healer will be here within the hour. Until then, the ash will continue to fall, and I will continue to ask that it not fall on the form. Twenty eris plus an offering. The eggs are a separate charge. (keeps her smile fixed and her hands folded beside the pen, gaze on the empty doorway where the runner vanished)


*Megumin sees.* The ash keeps falling like the sky itself is halfway through a sentence it cannot finish. Kazuma still has Aqua’s wrist in his grip, her tears drawing gray rivers down her cheeks, and the burned arm shakes but she does not pull away. Darkness stands there with the net sagging another inch, too focused on the wound to care. Luna stays neat beside her form and pen, calm as a ledger, while the eggs wait as their own small tribute. My half-circle lies at my boot, open and waiting, and I feel the weight of the unspoken ending pressing down on all of us.

*Darkness sees.* The smell of ash and scorched flesh is in my nose, and I am gripping this net as if it means something. Kazuma holds Aqua’s wrist, and I cannot stop watching his hand there. Megumin’s line in the ash is waiting, and I am waiting with it. Luna is too calm, and that calm makes my gauntlets feel heavier. I want the sentence finished, but I also want to feel the burn myself, to be the one pressed against the wound. I do not flinch. I would not flinch.

*Luna sees.* The ash drifts in slow gray curls, and I keep the form turned just enough that nothing lands on it. There is a half-circle scratched into the floor near Megumin's boot. Darkness's net has slipped again, and her gauntlets creak each time she tightens her grip. Aqua is crying through the soot on her cheeks, but her arm stays steady in Kazuma's hand, and the burn is red and raw in the lamplight. The pen lies parallel to the page. The doorway is empty where the runner disappeared, and the ash keeps falling while I wait.

*Aqua sees.* Ash keeps stinging my eyes and the burn on my arm is screaming, but I won't let go of Kazuma's hand until he says the words that make it true. Megumin is drawing shapes in the dirt like that will close anything; Darkness is fumbling a net she clearly doesn't care about; Luna is protecting a piece of paper like it's more sacred than me. Everyone is waiting for a sentence, but I am the one who stood in that mound and pulled every crawling thing out, and if he doesn't say it, then none of it ever happened.

*Aqua, privately.* If he won’t say it, I’ll make the wound say it for me—press his palm flat into the burn until the sting becomes the only true sentence in the room, and any flinch he makes gets filed as a confession.

*Darkness, privately.* Kazuma’s hand is already where mine wants to be, so I’ll take over and let the burn answer for me—if my thumb shakes there, it won’t be from her wound.

*Luna, privately.* I hate that I'm still waiting for one of them to do it; the burn does not care about signatures, and I know the healer's fee better than they do. I'll write the request in my own hand, mark it 'unable to sign,' and turn my counter sign to closed while I take her myself.

*Megumin, privately.* The sky is waiting for someone to finish the sentence, and I’m done letting Aqua’s tears write the last words. I’ll close the half-circle with my boot and give the marsh a name now, so Luna’s pen has to copy down a blast instead of a bill.


### 33

**Kazuma.** I head for the door, grabbing my jacket off the back of a chair and shrugging it on as I go.

"Megumin, Darkness — you two stay with Luna and make sure the eggs don't grow legs. I'm going to make sure Aqua actually reaches the cemetery and doesn't decide a bar looks more divine than a bunch of graves. Don't wait up."

I push the door open and step out into the ash-stained morning, calling back over my shoulder: "Keep that clean form, Luna. I'll owe you the twenty."

The door stands open on the white morning, and the ash comes in with it—fine and gray, turning in the light over the threshold, settling on the back of the chair where the jacket used to hang. Your shadow spills down the steps and breaks at the first stone, and for one long moment the whole guild seems to lean toward the gap you left.

Aqua does not follow. She stays planted in the settling ash, the burned arm still extended toward the empty space where your hand was, trembling so hard the charred cuff sheds a last thread of black. The tears keep coming, cutting clean lines through the gray on her cheeks, and her good hand clenches once at her ribs before she raises it toward the door.

Megumin's boot stays set in the line she drew through the fallen ash, and she watches the doorway with the flat stare of someone watching an omen walk off with its mouth still open. Darkness's net slips another inch; one corner of the mesh brushes the floor, and she does not retrieve it. Luna's pen lies exactly parallel to the form, and her knuckles are white around nothing.

**Aqua.** Kazumaaaa! You are walking to a cemetery that is already purified, you absolute— I purified every last mound, and the proof is seared into my arm! The only thing still unfinished is the sentence, and you are running away from it! (shouts after him, the burned arm still held out, ash drifting from her raised hand)

**Megumin.** The geography of his own order has defeated him. The cemetery is behind you, Kazuma — the omen is still in this room. A Crimson Demon does not flee a half-spoken invocation. She finishes it, or she stands in the ash until the speaker returns. (folds her arms, boot still planted on the line she drew)

**Darkness.** I am ordered to remain, and remain I shall! But if the speaker has fled, then I will stand in this doorway and take in the ash — every mote of it — until he returns to finish what he started. I would not move. I would not move even a little. (half-rises, the net sliding further through her fingers, eyes fixed on the empty doorway)

**Luna.** He is still within earshot, so I will say it once more for the record: the healer has been sent for, the fee is owed on arrival, and the eggs are a separate charge. Please do not leave the door standing open — the ash is settling on the only clean form I have left. (keeps her smile fixed on the doorway, pen exactly parallel to the form)


*Aqua sees.* The ash is still falling from my raised arm, and the proof of every mound I purified is seared into me — yet Kazuma is walking away from the finished cemetery, chasing some unfinished sentence instead of me. I see Megumin's boot planted on her line, Darkness half-risen with the net slipping, and Luna's pen frozen parallel to the form. The doorway is empty, and no one is kneeling or even turning back.

*Megumin sees.* The doorway is empty now, but the omen is not gone—it lingers in the ash that still sifts down from Aqua's raised arm, in the sag of Darkness's net, in the ridiculous perfect line of Luna's pen. Kazuma has fled his own geography, leaving a half-spoken sentence to rot in the air. I feel my line beneath my boot, scratched through the settled grime, and it is the only true thing in this room. Let him run. A Crimson Demon stands in her circle until the invocation is complete—or until the ash buries the cowardice.

*Darkness sees.* The doorway is empty where he walked out, and the ash keeps falling—settling on Luna’s clean form, on Aqua’s raised arm, on the net slipping through my fingers. I can taste the grit in the air. The others are speaking at his back; I am half-risen, ordered to remain, and the part of me that wants to chase the unfinished sentence is tangled in the mesh and the weight of the command. I will hold this doorway. I will let the ash touch me, every mote, and when he returns I will be exactly where he left me, unmoved.

*Luna sees.* I see Kazuma already at the doorway, and the ash is drifting onto the one clean form I have left. Aqua is still holding out her burned arm; Megumin has planted her boot on the line she drew; Darkness is letting the net slip through her fingers. The door is still open, and my pen is exactly where I left it. I keep my smile because the counter requires it.

*Aqua, privately.* He walked out on the wound and nobody turned, so I'll stop holding it out like a request. I'm going after him—not to make him look, but to stand inside whatever sentence he's chasing until it has to end with me.

*Darkness, privately.* The command still has me by the back of the neck—if I obey it too exactly, the stillness becomes the confession. Let them talk behind me; I’m busy measuring how long I can stay in the place he chose.

*Luna, privately.* Kazuma at the doorway isn't leaving—he's giving me a free witness. I'll put the healer request in his hand before he can turn, so the burn stops being my story and becomes his errand.

*Megumin, privately.* He left the door open, so now the whole street gets to hear it whether he wanted to or not. If I have to drag myself home through his absence, then the scrape of my boots will say what he ran from.


### 34

**Kazuma.** I quicken my pace through the falling ash, boots breaking the gray quiet, calling back toward the guild without turning. "I'll be back before the healer's bill does, Luna!"

The gate is there before you, and your hand finds the iron bar without meaning to. It is warm—not sun-warm, but warm like a stone that sat too near a fire—and the scorched smell of purification rolls down the hill with the ash, sweet and chemical, burned flowers and turned earth and the faint mineral edge of old bone.

You do not go in at once. Past the yew the graves have gone quiet, and in that quiet you can hear the ash settling on the gravel, a dry sound like sand through fingers. The front half of the burial ground lies open and gray, the headstones cracked and smoking at their seams, and no one follows you up the road. The grove stands still behind you; the town wall is a pale line in the falling light.

And there is Aqua.

She stands in the middle of the drift, one hand still raised as if the air has forgotten to let it down. The holy light has not quite left her fingers—it gutters there, a blue-white thread that drips and dies an inch from her skin—and her chest heaves under the dust on her dress. She sways once, catches herself, and points past the yew to the dark beneath it, where a last figure slipped back and the shadow now holds nothing but the slow turn of ash.

**Aqua.** Kazumaaaa! Now you show up! I purified every single mound in this cemetery all by myself — my arm is shaking itself to pieces and the ash is in my hair — and there is STILL one skinny shadow under that yew mocking me! This is YOUR fault! (keeps the raised hand pointed at the yew, swaying, the last thread of holy light running down her wrist)


*Aqua sees.* I’m standing in ash, my arm dying, and Kazuma finally shows up—after I did all the work. The holy light is still crawling on my skin but it’s dying too, and that shadow under the yew is still there, mocking me. The whole hill smells like burnt flowers and old bone, and my hair is full of grit. Of course he appears now, when it’s over and I’m too tired to even enjoy being right.

*Megumin sees.* The gate's warmth lingers on my palm like a stolen ember. Ash sifts down around me, pale as the dust of a world already ended, and the smell of the grove's labors hangs heavy — bitter flowers, scorched earth, a hint of something older beneath. Past yew and smoke, Aqua staggers in her own miracle, holy light bleeding from her fingertips, still pointing at a shadow that is already gone. I take it in: the devastation, the drama, the goddess trembling in the aftermath of her own overreach. A familiar scene — but the trembling is real, and so is the ash in her hair. My own staff rests easy in my grip. The shadows under the yew watch us, but they are only shadows now.

*Darkness sees.* The ash is in everything—my nose, my mouth, the hem of my gauntlets. I heard the clerk's clipped words about payment and the dry hiss of settling ash; I saw her smile stay fixed while her pen hovered. At the gate the iron felt too warm, and that burned-sweet smell rolled over me before I reached the hill. Then Aqua was there, swaying in the grey drift, a fading thread of light on her hand, shouting Kazuma's name with her whole body tilted toward the yew. I felt the exhaustion in her voice more than the blame.

*Luna sees.* Ash on my last clean form, the door left open, and Aqua’s voice carrying up from the cemetery — shouting about purified mounds and a mocking shadow. I kept my smile fixed and my pen parallel, but the air stings with burnt flowers and old bone.

*Aqua, privately.* Now he comes, when the light's just a stain on me and I can't even lift the arm to show what he didn't do. I'll make him carry the ash back with us—then he can tell the guild how late he was, and I won't have to say a word.

*Darkness, privately.* Her exhaustion isn’t blame, so I can’t take it the way I wanted—but I can carry it. I’ll get under her arm before anyone else does and let the heat pass from her to me, then hate that it still feels like a reward.

*Luna, privately.* The ash on the last clean form settles it: I can't keep this at the desk. I'll lock the door, keep the healer's fee in my apron, and go after Aqua before whatever she's shouting at comes to find better light.

*Megumin, privately.* The shadow is already gone, so the only thing left to exceed is her aftershock. If the grove thinks that little holy bleed was the cost, I'll show them what a real overreach costs.


### 35

**Kazuma.** The cemetery is already purified, and yet there's still one shadow left under the yew. I rip the iron gate open with both hands.

"I told you I'd come back, Aqua."

The silence does not last. Aqua's arm drops an inch, then snaps back up, and the thread of light along her wrist leaps and dies as she rounds on the gate—on you—with the whole weight of the burned morning in her face. Her chest is heaving, and every breath lifts a little cloud of ash from the front of her dress. She opens her mouth to speak, and for once the thing under the yew holds its place behind her, patient as stone.

**Aqua.** You told me you'd come back, and look at the state of you! Strolling through the gate while that thing under the yew just stands there watching me! I've burned my arm to a cinder purifying every last mound in this place, and it is still not enough for you, is it, Kazuma? (She sways, the raised arm shaking, ash spilling from her sleeve as she points at the yew without looking away from the Player.)


*Aqua sees.* I was still in the middle of the ash, my hand raised and my holy light dying on my wrist. The grove was behind me, the town wall pale ahead. Then I saw you come through the gate, and I turned on you, feeling every ash-coated breath shake my chest. My arm was trembling, and that shadow under the yew was still there, watching.

*Megumin sees.* I see Aqua standing in the ash, her raised hand still trailing a dying thread of holy light, ash dusting her hair and dress. Her voice is all raw exhaustion and indignation, and she keeps pointing at the dark under the yew even while she rounds on us. The whole scene smells of burned air and has the weight of a morning she has spent alone.

*Darkness sees.* Ash on my tongue, and the sight of Aqua swaying like a frayed flag — her arm held up not as a threat but as a plea the air won't answer. She burns herself out for a job no one thanked her for, and still the shadow waits under the yew, patient, as if it knows she'll be the one left standing in the dark. My hand itches toward my sword, but I feel the old heat in my own chest: the wish that something would look at me that way, with that same patient hunger, while I too shake and spill ash.

*Luna sees.* I take in the raw aftermath of a job gone solitary—the woman in the cemetery, ash-cloaked, her raised hand leaking the last of its holy light. She is not a threat; she is a familiar shape: the one who did the work and has no one to share the burden. Her voice is pitched to blame, but the shaking arm and the swaying body tell me the real message is 'I was left alone, and it cost me.' The shadow under the yew is just a prop in her exhaustion; what holds me is the state of her—a colleague of the battlefield, looking for someone to hold the weight for a second. The burned morning hangs in the air, and I read the ledger behind her words: time spent, effort spent, no one to notice until she broke.

*Aqua, privately.* I won't turn from the shadow just because he's finally at the gate. He can come the rest of the way and stand inside what he left me with—then either he flinches or I get to say I scared him more than the dark.

*Darkness, privately.* I keep telling myself it’s protection, but the truth is simpler—I want the shadow to shift its wanting from her to me. So I’ll cross to the yew and stand where Aqua stood, bait with a shield, and let the dark decide if I’m worthy of the same patient hunger.

*Luna, privately.* I stop reaching for the fee and reach for her arm first—if I make her sign before she can stand, I'm just another thing that asked too much of her tonight. She's done the work alone, and I can at least make the cost disappear for once.

*Megumin, privately.* She didn’t just wear a burn—she made one and stayed in it all morning, so the ash is already her altar before I’ve said a word. That means I’m not answering a stray spark; I’m deciding whether to name my blast over hers or beside it.


### 36

**Kazuma.** I'm at the cemetery gate, Aqua's here, we've met again. The fork already happened — she came here on her own while I was away. Now we're reunited, all on-stage together. Good. I just told her I'd come back. Let me close this beat — acknowledge her work, turn toward her, stay with this scene.

I pull the gate shut behind me with a clang of iron that rings up the hill and drops dead in the ash. I give the yew a long, flat look — then I walk toward her through the gray drift, boots crunching on the scorched gravel, and stop an arm's length from the swaying goddess with her burned hand still raised.

"Yeah. I'm back." I reach out and let my hand close around her raised wrist — gentle, not grabbing. "You did good, Aqua. You purified the whole place. Give me the hand and we'll get the healer to look at the burn. The shadow under the yew isn't yours to chase — I'll deal with it. You've done enough for one morning."

The clang rings up the hill and drops dead in the ash, and the cemetery takes it the way a stone takes a thrown coin—no echo, only the settling gray. You turn from the gate, and the whole burned morning is laid out before you: cracked headstones, ash sifting down the gravel path, and Aqua swaying at the center of it with her arm still raised, the last thread of holy light gone to nothing on her wrist.

Past her, under the yew, the shadow she has been pointing at does not move. It holds its place in the dark, a shroud-shaped stillness with a pale jaw showing behind its hair, and the ash drifts around it without touching. The graves behind you are quiet, truly quiet, in the way only newly scoured ground is quiet.

You start toward her, and your boots make the first new sound in the place—a soft crunch of ash over gravel—and the sound seems to steady her, or to unsteady her, one of the two. She sways again and catches herself with the heel of one sandal.

**Aqua.** Oh, NOW you close the gate, like you've locked us in here with it! Look at this place! I scoured every single stone until it smoked, and that one under the yew just stands there watching me—waiting for you to stroll in and witness the last little bit of my arm burn off! This is YOUR fault, Kazuma! (sways, jabs the raised hand toward the yew, ash spilling from her sleeve)


*Aqua sees.* The gate finally closed, and I can feel the ash in my throat and on my skin. My arm is a raw, burned thing, and that shadow under the yew has not moved once—it just watches. Kazuma walks in and the whole burned morning lands on me: he is late, the graves are scoured, and I am still standing here, small and burnt, with that thing waiting. I want to hit him for making me see how pathetic this looks.

*Megumin sees.* Aqua swayed at the center of the scoured cemetery, one arm raised and shaking, ash spilling from her sleeve as she pointed toward the yew. The shadow beneath it held still, a pale jaw catching the gray light. The whole morning smelled of burnt holy light and settled ash, and Kazuma's boots made the only real sound as he moved past me toward her.

*Darkness sees.* I stood behind Aqua, still and watchful, while she swayed with her burnt arm held out as if accusing the whole morning. I felt the ash settle around us and saw that silent shape under the yew keeping its patience, exactly as I kept mine. Then Kazuma’s boots broke the quiet, and I heard the moment change.

*Luna sees.* I see Aqua standing barefoot in ash, one arm raised and smoking, her sleeve shedding gray flakes as she points at the yew. The whole cemetery is scoured white-gray, headstones cracked, no wind. Under the tree a dark shape holds perfectly still, face half hidden, and the ash falls around it without settling on it. The gate clangs shut behind Kazuma; his boots crunch as he walks toward her, and that is the only sound. Aqua's voice is the loudest thing here—she is not whispering, and she does not look away from the shadow.

*Aqua, privately.* Don't hit him—if I hit him I'll start crying and then that thing gets to watch me shake. I'll make him stand in front of me and say he was late, then I'll decide if that counts as kneeling.

*Darkness, privately.* If I turn now, he'll see I'm not standing guard for Aqua—I'm standing here so the thing under the yew keeps looking at me. His boots mean I can get caught without asking, so I'll stay exactly like this and make him walk up to my back.

*Megumin, privately.* He goes to her because she swayed, not because she won. So I’ll make the shadow under the yew the first thing that moves, and when it does, his boots can turn around for the real ending.


### 37

**Kazuma.** I keep her wrist in my hand, thumb brushing the scorched cuff. "And I mean it — every mound, clear down to the bone. That's a job the guild would've charged a hundred eris for. Now let's find that healer before that arm decides it's a permanent souvenir."

Her wrist is hot through the scorched cloth—not fever-hot, but the deeper heat of something that has held holy light too long and is only now remembering how to be skin. Your thumb drags a fine black dust from the burned edge of the cuff, and the ash comes off on you, sifting down from her sleeve with every small shake of her arm. Where she sways, the last thread of light is gone entirely, leaving only the white of her face and two wet tracks through the soot on her cheeks.

Past her shoulder, the thing under the yew has not moved. The ash falls around it without touching it, so the shape keeps its clean dark outline against the gray, and the pale jaw hangs there with the patience of something that has watched this whole burned morning and reached no conclusion. Aqua's arm is still raised because your grip is what is holding it up, and her breathing comes in short pulls that stir the ash at your boots.

**Aqua.** I am not leaving! Not while that thing under the yew is still standing there with its pale jaw, watching me like I am the one who does not belong in a cemetery! You hold my wrist, you say a hundred eris, and then you want to walk out and leave it standing? Go stand under that tree yourself, Kazuma! (tries to wrench her wrist free to point at the yew, but only staggers and sags against his grip)

**Aqua.** And look—the ash will not even settle on it. It falls around it like it is scared. It is still mocking me, Kazuma, and you want me to go see some healer while it does it! (her voice breaks on the last word, and the bravado goes out of her, leaving only the shake in her arm)


*Megumin sees.* The smell of scorched cloth and something deeper, like copper left in the sun. Aqua's arm shakes in my sight, the burned sleeve shedding black dust that drifts and settles on the gray ground. Past her, the thing under the yew keeps its clean outline, untouched by the falling ash—a pale jaw hanging with a stillness that feels older than the morning. Aqua's voice cracks, and I see the wet tracks on her cheeks beneath the soot. The heat from her wrist reaches me even from here.

*Darkness sees.* My senses are full of ash and the smell of burnt cloth. I feel Aqua's wrist in my grip—hot, trembling, and held up only because I am holding it. I see her face streaked with soot and tears, her bravado collapsing into shaking breath. Across the gray air, the shape under the yew stays clean and unmoving, as if the falling ash refuses to touch it. I hear her ragged breathing and the small sift of cinders around our boots.

*Aqua sees.* He is holding my wrist, and the heat in my arm is not just pain—it is the leftover of holy light that hasn't fully let go. The ash keeps sifting off my sleeve and gathering around my boots, but the thing under the yew stays clean and patient, as if my burning morning means nothing to it. I am shaking. The wet tracks on my cheeks feel like dirt lines, and the bravado I tried to hold is gone, leaving only the ache of being seen like this.

*Luna sees.* The smell of burnt cloth came in with them before the words did—that sharp, holy-fire tang that lingers in the guild hall long after the quest board is wiped clean. Aqua's sleeve is a wreck, black at the cuff, and she sways like she's still feeling the shock of it. Kazuma's grip on her wrist is the only thing keeping her upright, and the ash keeps sifting off her with every small tremor. Past her, the door frame frames the gray morning, and the report she's shouting mentions a yew and a pale jaw, something that wouldn't let the ash touch it. I note the burns, the exhaustion in the shake of her arm, and the way her bravado thins into something smaller before it breaks. Another mess to fold into the ledger. Another adventurer who came back less whole than they left.

*Aqua, privately.* If he lets go before the light finishes, the shadow gets to watch me fall and then I'm not the thing it bowed to—I'm just another wet stain in the ash. So I'm holding on, but I'll call it making him steady me.

*Darkness, privately.* Her wrist is hot and wet and I can feel every finger of my own hand on it like a brand I begged for. The shadow under the yew hasn’t moved because it’s watching me hold her up—and I’m not bait now, I’m the thing it’s waiting to break by making me let go.

*Luna, privately.* The yew and pale jaw settle it—this isn't a burn to bill, it's something holy fire wouldn't take. I'm not reaching for the quill; I'm locking the door and checking if it followed them back.

*Megumin, privately.* That heat is her skin still cooking, not a relic, and the yew hasn’t even blinked—so I’ll stop sorting over or beside and give it one reason to look at us both: a blast that uses her burn as the fuse.


### 38

**Kazuma.** I'm here with Aqua at the cemetery, reunited. I made my promise — now I follow through with it.

I keep her burned wrist in my hand, thumb anchored on the scorched cuff so she can't wave it at that shadow again.

"All right, you win. One stubborn shadow under the yew, all yours." I glance past her at the dark shape that still hasn't moved, then back at her sooty face. "But first — that healer I promised. Arm's shaking. Come on, before you need a new wrist instead of just a bandage."

The ash does not settle on it. You can see that now, watching past the rise of her shoulder: the gray falls in a slow curtain and parts around the thing under the yew, as though even the dust has learned to keep its distance. It has not moved. It stands in the black beneath the branches with its pale jaw tipped toward the two of you, patient in a way the morning is not.

When you look back, her face is a ruin of soot and wet shine, and the bravado that carried her across the front half of the burial ground is gone from it. Her free hand twitches toward the yew and drops. The wrist you hold stays hot through the scorched cloth, and every breath she takes lifts another small cloud of ash from the front of her dress. For a long second she only sways against your grip, her mouth working as if the words have to climb out through all that dust.

**Aqua.** Don't you dare 'but first' me, Kazuma! I am not walking to some healer while that pale-jawed thing is still standing there under my yew! You said one stubborn shadow—one—and that one is MINE, and you are going to stand right here and watch me finish it, and then... and then you can carry me all the way back and tell the guild how late you were! (jabs her free hand toward the yew, then lets it fall; her voice climbs and cracks on the last word)


*Darkness sees.* I saw Aqua's posture finally break—the arm she wrenched toward the yew was a flag coming down, and her voice cracked past all its own bravado. I saw the ash falling in a gray curtain that parted around the thing beneath the branches, as if the air itself refused to touch it. I saw her wrist burning under scorched cloth in Kazuma's grip, and the tremble in her arm that had nothing left to hide behind. The thing stood patient, pale-jawed, and I felt the weight of her claim to it—she named it hers before her voice gave out.

*Aqua sees.* I saw the ash falling like a curtain past my shoulder, and that pale-jawed thing still standing under MY yew, untouched by it, patient, as though even dust has learned to insult me. I felt your grip locked around my wrist, hot through the scorched cloth, and my own arm shaking, begging to break free—but I only staggered, sagged against you, my dress lifting a fresh puff of ash with every breath. I saw the wet shine on my own face, the soot, the ruined front of me, and I heard the words climb out crack-voiced and dusty, and still that thing would not move. You told me one stubborn shadow—one—and it is MINE, and I am not walking away while it stands there, Kazuma. I am not leaving it standing under my tree.

*Megumin sees.* From where I stand, this is less a burial ground and more a stage set for a duel of wills—the gray air, the weeping goddess, the steadfast shadow. Aqua burns with a refusal that I recognize, a defiance that will not be healed or led away. The thing under the yew does not flinch; it waits like a worthy antagonist in a tale, patient even as the world falls around it. And Kazuma, who pretends to carry the burden of sense, holds her wrist as if he does not know the true weight of her stubbornness. I feel the urge to applaud her audacity, to name this moment something grand, though I suspect my own hunger for climax may be the only thing louder than her cracking voice.

*Luna sees.* I take in a scene of ash and stubbornness: Aqua, soot-covered and shaking, trying to wrench free from Kazuma while her voice breaks over a yew tree. Something stands under it—pale-jawed, still, with ash falling around it as if the air itself keeps away. She's calling it hers, refusing to leave for a healer, and promising to drag Kazuma back to the guild with a late report if he doesn't stand and watch.

*Aqua, privately.* It didn't shift an inch, and I felt my voice go dull against its face like a thrown stone. So I'll keep his hand on my arm, because if I can't make it kneel I at least need him to believe I still could.

*Darkness, privately.* She named it hers, and that flips what I am: not her shield, but a thief standing inside her claim. I’ll still put myself between her and the yew—and I’ll hate that I want its gaze to choose me over her safety.

*Luna, privately.* Not a healer call anymore—if I try to pull her, I'm just the next hand she shakes off, and what's under the tree will let me be. I'll keep the door open and start the report as 'caster refuses aid, cause under yew,' so the temple has to take her claim, not me.

*Megumin, privately.* I won't clap. If I do, I'm just another voice in the crowd, and the shadow gets to stay the patient one. I'll let Aqua's crack be the opening, wait until the yew thinks that's all we have, then spend my hunger so it can't look anywhere else.


### 39

**Kazuma.** I glance at the thing under the yew once more, then shift my grip on her scorched wrist so the weight of my hand steadies hers.

"Stay right here. Don't move, don't wave that arm at anything, don't try to finish it off solo." I start walking toward the shadow, boots crunching the ash, one hand already reaching for the hilt of Chunchunmaru. "I'm taking this thing's head off so YOU can go see the healer without arguing, you stubborn—"

The ash closes over your tracks as soon as you stop, gray filling the hollows of your boot prints, and the thing under the yew still has not breathed. Its pale jaw hangs open now by a thumb's width, and the dark inside it shows for a moment before the shadow closes again. Behind you, Aqua's voice goes high and ragged, and the sound of it makes the stillness under the tree feel deeper, not broken.

The cold off the bare ground climbs the shin of your boot like a second shadow. Your fingers have not left Chunchunmaru's hilt, and the hilt has not warmed. The creature waits the way the ash waits to fall—without impatience, without sound, its dark hair unmoved though Aqua's shout cuts the air behind you like a thrown stone.

**Aqua.** Don't you DARE steal it, Kazuma! I purified every single grave in this cemetery, and that pale-jawed thing is MY purification! If you so much as nick it first, I'll tell the guild you tripped over a headstone and I had to save you! (sways and jabs a finger at the yew, then clutches her scorched arm and hisses through her teeth)


*Aqua sees.* My arm is burning and my voice is shaking, but I will not let him take this from me too. The ash is on my tongue and all over my dress, and that pale-jawed thing is still there, not breathing, not moving, like it is waiting for me to crumble. Kazuma is holding me up, but his hand is on that sword, and I know what he is thinking. I did every grave. That purification is mine. He can carry me later, after I finish it, after the guild sees what I did.

*Megumin sees.* I watch from the ash — Kazuma holding Aqua upright, her scorched dress breathing out little clouds of soot with every word. The thing under the yew has not even blinked; it waits like the ash waits, patient and cold. Aqua's voice cracks as she claims it for her own, and for a heartbeat the whole cemetery feels drained of sound.

*Darkness sees.* The ash is thick enough that I can taste it on my lips, and the cold from the ground is climbing up through my boots. Kazuma has hold of Aqua's wrist, but she is pulling toward the yew, burning, and the thing under it has not moved once. There is a stillness down there that feels deliberate, like it is waiting for us to be exactly where we are right now. My hand is on Chunchunmaru, but the hilt is cold, and that cold tells me more than any shout could.

*Luna sees.* Ash hangs in the air and keeps sifting down; I can smell scorched cloth and old cemetery dust. Aqua's voice is loud, ragged, cracking on the last word, then turns into a sharp hiss of pain. Under the yew, the pale-jawed shape stays absolutely still, and the shadow over its open mouth closes again without sound. The cold off the ground is rising, and I notice Kazuma hasn't taken his hand off Chunchunmaru's hilt.

*Darkness, privately.* Let go of the hilt. It's not a weapon for this—the cold has climbed into my arm and told me the thing under the yew is a threshold, not an attack. Aqua is already leaning across it, and if I don't move first I'll be holding her back from the very dark I wanted to enter before her.

*Luna, privately.* Kazuma's hand on the hilt turns it from a medical call into a fight, and I have no column for that. I'll get Aqua inside before the shape decides it has a reason to move.

*Megumin, privately.* A claim that cracks is a fuse already wet. The thing under the yew heard that and didn’t blink—so if I borrow her burn, the silence just swallows both our names. I’ll say it dry, alone, and make that cold wait answer to me, not to her cracked note.


### 40

**Kazuma.** I stop mid-step as the shadow under the yew finally breathes.

The breath does not stop. It settles into the creature like a slow tide, and with each exhale the dark beneath the yew seems to lean a little farther out over the path, though the figure itself never shifts. Aqua's sandal scrapes again, closer now, and the heat of her raised arm reaches you near the shoulder as she comes up behind—not to help, but because whatever stands in front of you has finally given her a reason to stop being the only thing still moving.

**Aqua.** Kazuma, it breathed! It BREATHED at us! Do not tell me you did not hear that—my whole divine body just went cold! Finish it, or hold it still, but do NOT just stand there with your boot in the air! (clutches her scorched arm and points a shaking finger past his shoulder toward the yew, voice cracking)


*Aqua sees.* The cold was climbing into my boots, and I could see the gray filling the prints behind him. That thing had gone still for too long, and when the dark under the yew opened just a sliver, I felt the air go wrong. My own shout came out ragged, and the scorched arm throbbed when I pointed. Then it breathed—one long, slow pull—and the whole space under the tree seemed to lean forward like it was tasting us. I moved up beside Kazuma because I couldn't stand being the only one still moving while that thing waited.

*Megumin sees.* The cold off the gravel pressed through my boots, and I kept my staff ready without actually raising it. Aqua's voice came in jagged bursts behind Kazuma—cracked, indignant, and a little too loud for a cemetery. The creature under the yew barely moved, but its breath was unmistakable, a slow tide that made the shadows lean outward. I saw its pale jaw gape, then close again, and for a moment the whole scene felt like a verse waiting for its final rhyme: Kazuma frozen, Aqua scorched, and that quiet patient dead thing beneath the branches. My fingers itched for a bigger answer, but the moment wasn't mine yet.

*Darkness sees.* Kazuma stands with one boot lifted, hand on Chunchunmaru, watching the pale-jawed creature under the yew. Aqua's voice is high and ragged behind him, but the silence under the tree only deepens. I take in the open jaw, the shadow inside it, the slow breath that finally comes. The cold climbs my own shin like a second shadow, and I find I want the wait to stretch—not to end, not to be resolved, just to stay there between breath and strike.

*Luna sees.* The cold that drifted in with the evening report had the feel of a complaint filed late — the kind that comes with scorch marks and a raised voice. I heard Aqua's shout before I saw the two of them at the edge of the grounds, and I knew without asking that the quest would need a second form: one for the creature, one for the collateral. The thing under the tree was nothing special — an undead with a sluggish pulse — but the pause before the sword came out was the part I was trained to read. It meant someone was about to make this my problem instead of theirs.

*Aqua, privately.* Not a bow—a breath, and for the first time I don't want the shadow's face on me alone. I'll stay even with Kazuma, close enough that if it lunges we go down as the same target and I'm not the one left standing in its mouth.

*Darkness, privately.* Kazuma’s hand on the hilt is the only thing I fear, not the open jaw—if he strikes, the moment collapses into mere rescue, and I’ll never learn what the shadow’s patience wanted from me. I settle my heels and keep my own hand open, willing the cold to know I’m not here to end it.

*Luna, privately.* They're waiting for the creature to be my problem, so I'll name it before they can. I'll call it a suppression request from the doorway and put their names on it before the sword can turn pause into paperwork.

*Megumin, privately.* That open-close jaw isn’t waiting for Aqua—it’s tasting how much heat we’ll waste before it bothers. Fine. I’ll wait until it inhales again, and then I’ll say the first word into that vacuum, so its own breath has to carry my name instead of hers.


### 41

**Kazuma.** The thing under the yew just breathed. Aqua is melting down beside me, scorched arm shaking, blaming me for all of it. I couldn't possibly walk away from this to go find a fork elsewhere — I'm already at the cemetery, already in the fusion point. The threat is right here in front of us.

I stop mid-step, Chunchunmaru still in hand, and the shadow under the yew keeps breathing slow and unhurried.

"Alright. You want to claim it, it's yours — but you're not going near it with that arm, so we're doing this together." I keep my eyes on the black under the branches. "Back me up with light from a distance, and I'll hold it still enough for you to hit it clean. One shot, then we're both walking out of here and you're seeing a healer, no arguments."

The second breath comes as you watch, and it does not fog the air the way living breath should. It parts the ash instead, a slow exhalation that pushes the gray outward in a soft ring around the yew, then draws back in taking nothing with it. The pale jaw tips a fraction wider, and the dark behind it simply holds.

Chunchunmaru is a live weight in your hand. The hilt has warmed through from your palm, and the edge catches no light at all, just a line of dull steel against the black beneath the tree. Behind you, Aqua's scorched arm presses at your shoulder without her meaning to, and the heat of it seeps through your shirt like a warning.

The thing does not come out to meet you. It stands at the edge of the shadow and breathes again, unhurried, the way the tide breathes against a wall, and the whole burned cemetery seems to be waiting to see which of you moves first.

**Aqua.** Kazuma, move! It's breathing AT us, and you're holding that sword like it's a holy relic! (She clutches her scorched arm and takes a half step back, her voice cracking on the last word.)


*Aqua sees.* I heard it breathe—not like a living thing, but something that pushed the ash out and pulled it back in like a tide. My arm is burning where I got scorched, and the heat is pressing through Kazuma's shoulder as I grab at him, but he's just standing there with Chunchunmaru like it's a relic. The whole burned cemetery feels like it's waiting, and I'm terrified and furious at the same time because he's making me be the one who sees what's in front of us.

*Megumin sees.* The air is thick with ash and the weight of a held breath. I watch Kazuma stand like a fool in his dramatic pose, boot suspended as if the moment itself had caught him mid-thought. Aqua shrieks and dances backward, her arm scorched and steaming, her divine panic a shrill counterpoint to the silence under the yew. The thing there breathes in long, slow tides that push the ash outward in perfect rings, then draw back empty—and the pale jaw opens another sliver to show nothing but hungry shadow. My own breath catches. This is not a beast to be carved; it is a presence that demands erasure in the grandest gesture I possess. My fingers twitch toward my staff. Soon. The moment must ripen.

*Darkness sees.* I heard the slow, deliberate breath of something under the yew—dry, not living, pushing the ash outward in a ring and then drawing nothing back. I saw Aqua move closer, clutching her scorched arm, her voice cracking as she urged action. I felt the whole burned cemetery holding still, the dark beneath the tree leaning out with each exhalation, and Kazuma frozen mid-step, sword in hand, his boot still raised while that pale jaw widened and waited.

*Luna sees.* The guild records describe this cemetery as a place of rest. Nothing about that description matches the cold, rhythmic breathing coming from the shadow under the yew, or the way the ash parts and settles with each exhale. The Adventurer's Guild protocol for undead encounters specifies decisive action, not frozen stalling. Lord Kazuma stands locked in place with his blade raised, a posture that suggests either great deliberation or complete paralysis, both equally inconvenient for a quest file. Miss Aqua's state has deteriorated beyond mere sobbing—the scorched arm she clutches is a visible injury pattern, and her account of the creature's breathing is consistent with what I can observe from here, though her divine claims remain unverified. The sword edge catches no light, which is unusual for properly maintained steel. The dark holds where the jaw tips open, and nothing about this scene aligns with the quest description I filed this morning.

*Darkness, privately.* Aqua’s cracked order breaks the stillness, and for one second I’m furious that she’s the one the dark will answer. I step forward into the ash before I can choose, shield up but sword down, putting my back to her and my front to that open jaw—if it wants a fighter, it’ll have to go through the side of me that still wants to kneel.

*Luna, privately.* If the file is that wrong, then the safety margins I’ve been enforcing are just paper too. I put the ledger between me and the door and start watching the shadow, not the signatures.

*Megumin, privately.* It isn’t waiting for my name—it’s already breathing us in, and Kazuma’s frozen boot is sitting inside the next ring. I’ll give it an exhale it didn’t ask for and cast now, before that jaw learns to close on something with a name.


### 42

**Kazuma.** I tighten my grip on Chunchunmaru and take one deliberate step closer to the shadow under the yew, keeping Aqua's good shoulder between her and the path back to the gate.

"Light it, Aqua — now, from back here. I'll go in and hold it still, you hit it clean, then we're out."

You step past the last clear air and into the shadow's lee, and the cold does not touch your skin so much as it settles behind your eyes, the way a held breath goes hard in the chest. The thing under the yew turns its pale jaw toward Chunchunmaru's point as though it knows the blade better than the hand that carries it. Ash still will not settle on it. The gray hangs around the figure in a suspended sleeve, and the sleeve turns slowly as the dark beneath the branches gathers.

Behind you, Aqua's good arm comes up. The light does not come at once. There is a dry click of air, then a wobbling glow that climbs her fingers like water finding a crack, too white for the gray morning, and the yew's shadow peels back from the path. Her aim swims. The light rolls to the end of her hand and waits there, dripping.

You close the distance. The figure's head tilts, the jaw a finger's width wider, and then it is not breathing at all—it is stepping, and the ash beneath it still has not settled, and the first of its shadow touches the point of your sword before any sound reaches you.

**Aqua.** Kazuma, what kind of plan is 'now'?! I am not steady—and it is moving, it is MOVING— fine! If I miss, it is your fault, and I am telling the guild you jumped in front of a goddess mid-smite! (raises her scorched arm, light wobbling and flaring white)


*Aqua sees.* Kazuma is walking straight into that thing, and my arm is throbbing, the light sputtering like a candle in a storm. The cold is creeping into my skull, and the ash just hangs there, mocking me. I can barely keep the glow steady, and he calls this a plan? If I miss—when I miss—it's his fault. He's making a goddess stand here shaking while that shadow steps closer. I can feel the weight of the yew pressing on me, and my light is dripping off my fingers like it knows it's useless.

*Megumin sees.* I watch the ash refuse to fall on the pale jaw, and I think: this darkness has manners. It knows the blade. It does not know me. That cold behind the eyes is the only greeting it offered, and I have met ruder introductions. The goddess's light wobbles like a candle in a draft, and I feel the glory of our desperation settle into my chest, warm as a cast-off spell. That thing is not breathing—it is deciding. Let it decide. I have made my choice already, and it is as beautiful as a pillar of fire.

*Darkness sees.* The cold found its way under my armor and sat where I keep my composure. The ash hung on the air even around my own gauntlets, and for a moment I wanted to press into that shadow instead of watching Kazuma take it. Aqua's light bloomed too late and too unsteady—magnificent. My pulse did not race from fear. It raced because someone else was offering himself to that hunger, and I was only allowed to stand and witness. I did not move forward. I wanted to be moved through.

*Luna sees.* The cold slipped past skin and settled somewhere behind my eyes—I've felt that before, in winter queues when the guild doors stick. His stance was all wrong: sword held like a relic, feet planted like he was waiting for permission. Behind him, the blue one's light came up too slow, wobbling like a lamp about to die, her scorched arm shaking with the effort of aiming something that clearly wanted to be elsewhere. The shadow under the yew did not rush. It moved the way the last queuer does when you call closing time—patient, knowing the hour already belongs to it. I noted the ash that refused to fall, the way the air went hard in my chest, the sound of a goddess bargaining with a man who would absolutely take the blame. Somewhere beyond this, there is paperwork: property damage to the cemetery, one burned arm, one blade possibly cursed, one goddess who will file a complaint in triplicate. I filed it all mentally before the first step landed.

*Aqua, privately.* Maybe I'll just drop the light entirely and let him see how dark it gets without me—then he'll have to turn around and put it back in my hand himself.

*Darkness, privately.* I won’t stand witness while Kazuma takes the hunger I waited for. I let my shield arm sag a finger’s width, a quiet invitation only the shadow will read, and I stay close enough that the next cold mouth finds me first.

*Luna, privately.* The way it waits like closing time tells me the ledger won't hold it; I put it down and want Aqua behind me before that first step lands, because we're past fees and into casualties.

*Megumin, privately.* That cold isn't contempt—it's the one set of eyes in this grove that hasn't learned my name, and I won't let that stand. I'll cast before it decides whether to close, so the only name the dark can mumble back is mine.


### 43

**Kazuma.** I step forward, blade low and steady, and call back over my shoulder without taking my eyes off the pale jaw.

The figure stops breathing. The next thing that reaches you is not breath but shadow: it spills out from under the yew, thin and quick as poured water, and touches Chunchunmaru's point first, black against the dark steel where no light catches anyway. The cold that had settled behind your eyes tightens, and the blade's hilt turns hot in your hand as though the steel has finally recognized what it is being asked to face.

It steps. Not a lunge, not a shamble — one deliberate step that pours its shroud from one patch of black into another, and the pale jaw tips a fraction wider as it comes, close enough now that the gray ash at its hem hangs motionless instead of falling. Behind you, Aqua's light swells in a raw, wobbling pulse that throws your shadow flat against the ash and prints the creature as a black cutout against the yew, and then her arm wavers and the shape dissolves back into the dark.

The ground under your boots is still, and the thing in front of you is not. It stands just beyond the reach of the blade, swaying the way a curtain sways when the window behind it is open, and waits for one of you to move first.

**Aqua.** Kazuma, it STEPPED! It stepped, and its shadow is on your sword, and I am NOT steady! On three, remember? ONE— no, I already lost count, just hold it STILL, hold it still and stop looking at its jaw! (she braces her scorched arm with her other hand, light flaring white and shedding sparks into the ash)


*Aqua sees.* That thing moved before I was ready—its shadow slipped onto Chunchunmaru like it was tasting the blade. My light won't hold steady, my arm is burning, and every spark falling into the ash feels like I'm losing pieces of myself. Kazuma is just standing there, staring at its jaw, and I need him to keep it still because if I miss, it will absolutely be because he flinched. It sways like a curtain, and the ash around its hem doesn't even fall naturally. I am a goddess, and I am not supposed to be this scared in front of a monster.

*Megumin sees.* The shadow found Chunchunmaru before the sound did—black against black, and the hilt turned hot as if the steel finally knew what it was supposed to bite. The thing took one deliberate step, not a lunge, and the ash at its hem stopped falling. Aqua's light kept stuttering behind me, throwing my shadow flat and then letting the dark pour back in. The ground was still. It was not. Everything in me wanted to chant the incantation now, and that was exactly the problem.

*Darkness sees.* I took in the shadow touching the blade before any sound, Aqua's light stuttering into sparks, and the creature stepping close with its jaw wide—held in a stillness that made the ash feel frozen around us.

*Luna sees.* I saw the shadow reach the blade's point before any sound came. Aqua's light flared and stuttered, scattering sparks. The shrouded figure stepped once, close enough that its hem stopped moving, and the air turned still and cold.

*Aqua, privately.* If I wait until I'm not shaking I'll never cast, so I'll aim at the mouth that touched his sword and let the light go before he can flinch—if it bends, it's because he moved, not me.

*Darkness, privately.* It never wanted me—it wants her, and my slack shield-arm stops being an offer and becomes a gap. I bring it up too late and hate that the heat in my chest is fear now, not the hunger I was begging to meet.

*Luna, privately.* It stepped past the sword, not because it was unafraid, but because the steel was never the point. I'm done with thresholds—if it moves again I'll pull Aqua back by the collar and put the door between her and it.

*Megumin, privately.* The hilt going hot means it is trying to arm me on its own timetable, and if I cast now I’m only the second weapon it picked up. I’ll hold the chant behind my teeth until the impulse hurts like it’s mine, not bait.


### 44

**Kazuma.** The brief says: if there's an off-stage storyline, that's the only job this turn. But here, everyone who matters is on-stage with me. Wait — there are no off-stage storylines in the list. Only the konosuba scene is live, and Wiz is "not on this stage" but has no separate scene.

Per the brief: **Since everyone who matters is on-stage, split once this turn — send Aqua only.** But wait — Aqua, Megumin, Darkness, Luna are all on-stage with me, and driving intention references the cemetery mission already in progress with me present. We're already IN this scene at the cemetery fight.

Hmm, let me reconsider. The scene is at the cemetery (the narration describes the yew, ash, burial ground). Aqua is here with me. The brief's split instruction: "Send Aqua only to Axel's cemetery to purify the undead." But we're already at the cemetery fighting. 

Wait, the driving intention says "Sulk-drunk but on a divine mission, trudge to the cemetery and purify the undead the Player blamed her for attracting." We are apparently already AT the cemetery, mid-fight with this pale-jawed figure.

So the split already happened in the story — Aqua is here with me fighting. There's no off-stage storyline to merge into. The brief's split instruction applies "if everyone who matters is on-stage" — but we're already in the middle of the cemetery fight together, and splitting now would be nonsensical (sending Aqua away from the battle she's mid-purification in).

Actually, re-reading: The brief is a force of fork-then-merge. But the current scene already has everyone together mid-fight. There's no off-stage storyline. So I should handle the immediate scene: the fight with the pale-jawed figure.

Given my previous action — "I step forward, blade low and steady" — I'm engaging the creature. My current situation is mid-combat. The brief's directive about sending Aqua to the cemetery is already fulfilled/moot since we ARE the cemetery. I should continue the fight.

Let me commit to my action — I stepped forward with blade low. Continue engaging.

I face the creature. My action line:

I close the last of the distance, swing Chunchunmaru up in a rising cut at the spot where its jaw meets shadow.

The shadow does not stop at Chunchunmaru's point. It climbs the flat of the blade in a thin, black film, moving against gravity like water drawn up a wick, and where it passes the steel turns from warm to scorching under your fingers. Behind you, Aqua's holy light hisses and gutters, spitting white sparks into the ash, and her breath comes in short wet hitches that fog the air her own magic cannot settle.

The figure under the yew still does not move its body. But its pale jaw works once, slowly, as though testing the hinge, and the dark pooling at its feet begins to climb the trunk behind it. The yew's needles shiver without wind and shed a fine gray dust that is not ash and does not fall.

Aqua's sandal scrapes back half a step, and the sound is too loud in the hush. The hilt is hot enough now that your palm knows it, and the shadow has reached the crossguard.


*Aqua sees.* I took in that thing moving against its own stillness, jaw testing the air, and the wrongness of its shadow crawling up Kazuma's sword while my light kept sputtering—I couldn't steady my arm, and my own breath sounded too wet and loud in my ears.

*Megumin sees.* From my seat in the ash, I took in the whole ugly tableau: Aqua's holy light surging and guttering in raw white pulses, Kazuma's sword silhouetted against it with a black film crawling up the flat like spilled ink climbing a wick, and the pale-jawed thing under the yew swaying without moving its feet. The heat off Chunchunmaru reached me even here, and the gray dust the tree shed hung in the air as if the world itself had forgotten how to fall. Aqua's breath kept hitching, her sandal scraped back, and the hush was so loud I could hear the sparks die in the ash.

*Darkness sees.* The ash hung still and my breath went quiet with it. I saw the shadow drink the length of Kazuma's sword, and I heard Aqua's light shiver behind him. The creature's jaw worked like a hinge, and I felt the threat settle low in my chest—not fear, but a kind of readiness. I wanted to be between it and them, to take whatever that stillness was hiding. The wanting and the duty were the same heat.

*Luna sees.* I am not there, but the report is enough to feel it: a shadow that crawls up steel, Aqua's light sputtering, a yew that sheds dust without wind. Something is wrong beyond a normal monster — this will not end with a simple quest report.

*Luna, privately.* That means the old columns won't hold it, and I'm done pretending a clean form is the same as a safe town. I'll close the desk, get the original requester's name, and go see the yew myself before it becomes someone's third-hand quest.


### 45

**Kazuma.** I close the last of the distance and swing Chunchunmaru up in a rising cut at the seam where its pale jaw meets the shadow, putting my weight behind it and letting my heels bite the ash.

The rising cut finds its seam.

Chunchunmaru's edge meets the place where the pale jaw hangs against the dark, and for a heartbeat there is resistance—then not. The black film on the steel flashes once, white-hot, and tears apart like a skin too thin for the heat beneath it. The figure does not scream and does not bleed. The jaw tips back, opens a finger's width, and the shadow that was its body splits along the line of the cut, peeling away in two long sheets that try to close again around the blade.

Behind you, Aqua's light does not gutter. It gathers—white, then blinding—and she lets it go with a sound that is more sob than incantation. The light does not arc. It arrives, a column of heat that fills the split shadow before it can knit shut, and the thing under the yew comes apart from the inside, black shredding into gray, gray whitening to ash.

The ash does not drift. It drops all at once into a soft pile at the foot of the yew. The heat goes out of Chunchunmaru so fast your fingers ache with the cold that follows it. The yew stands empty, its lower branches still and unscorched, and the pale jaw is gone. What remains is ash, a smell like burnt rainwater, and Aqua's ragged breathing close behind your shoulder.

**Aqua.** Did you see that?! That was ME! One perfect purification, straight down its nasty shadow-throat — and you were just the anvil, Kazuma! If you'd twitched even a little, I would have burned your whole sword arm off and it STILL would have been your fault! (lets her scorched arm fall, wobbles, then stoops and digs a stone angel's head out of the ash, clutching it to her chest like a prize)


*Aqua sees.* The light tore out of me like something I couldn't hold back, and the dark split and frayed until it was just gray ash falling straight down. My arm is screaming now — scorched from my own miracle, maybe, but it was worth it. I can hear myself breathing too loud, and Kazuma is still standing, sword lowered. I stooped and dug this little angel head out of the ash. It's warm. I'm not letting go of it.

*Megumin sees.* My palm still remembers the hilt's heat, then the sudden cold that bit after the light. The cut felt right — a perfect seam — but it was not my explosion that ended it. The shadow tore apart and became ash at the foot of the yew, and behind me that noisy priestess was already claiming the glory. I watched her fish a stone angel from the ash, and though my legs were steady for once, the moment tasted like someone else's spell had stolen my finale.

*Darkness sees.* The cold biting my fingers where the heat had been, the weight of the ash settling like a held breath finally let go, and Aqua's voice ringing too bright, making sure the victory was hers—while I was left watching the sword in my hands, the work already done, unacknowledged.

*Luna sees.* They came back with the stink of burnt rain clinging to their clothes, Aqua's arm red and blistered but her voice loud enough to shake the dust off the notice board. Kazuma was quieter, flexing his sword hand like he'd grabbed ice, and the blade at his side had a dusty sheen that didn't look like any normal wear. She was cradling that little stone angel's head like a trophy, already demanding celebration drinks, while he kept glancing at the sword as if it might speak. None of them asked about the absence of a quest slip. They never do when it goes like that.

*Aqua, privately.* The light didn't just answer—it used me up, and now the only thing left that's warm is this little stone face. I'll make him carry it back, not because my arm's ruined, but because he owes the miracle one more hand than it got.

*Darkness, privately.* I'll let Aqua take the glory and test whether that feels like being shielded or being left outside. Then I'll walk back to the yew before sleeping, not to fight—just to feel the cold again on the hand that failed her.

*Megumin, privately.* I stayed on my feet because I didn't spend anything, and now the ash is her altar instead of my afterimage. I'll make the next crater swallow that little stone angel before I let her keep the ending.


### 46

**Kazuma.** The blade tears through the shadow-sheets clean, and Aqua's column of light finishes what my steel started. The yew stands empty. Ash settles. I lower Chunchunmaru, heat leaving the hilt, and look back at her over my shoulder.

"See? Teamwork. You singe, I swing. Now check the ash pile for anything shiny before you pass out on me."


*Aqua sees.* I felt my own light gather until it was too bright, and the sob came out of me as the column slammed into the shadow that Kazuma's blade had split open. The heat was real, and the thing came apart in grey and white, dropping into ash. Then the cold bit into my fingers, and I was breathing too hard, my scorched arm down, the world tilting. I saw the empty yew and grabbed the little stone head out of the ash — mine now.

*Megumin sees.* The light was beautiful—more beautiful than any explosion I have cast, and that burns me more than the heat ever did. She stole the finale. I stood frozen as the shadow tore apart, not from the spell but from the sight of it. Now my fingers are cold and Aqua is clutching a stone angel head like a trophy, calling me an anvil. I want to argue, but my voice is still caught in that column of white.

*Darkness sees.* I saw the cut land true, and the shadow gape open like a thing that had never expected to be touched. The light that followed was not Aqua's usual noise—it was a clean, blazing column that unmade it, and for one breath I felt the sword go cold in my hands, as if the battle had already left us. The ash fell straight down, and the yew stood quiet, and I was aware of my own pulse, loud and almost disappointed that there was nothing left to shield anyone from.

*Luna sees.* The hush after the strike felt heavier than the fight itself. I saw the blade flash white-hot, heard the tearing sound that was not quite cloth, and then Aqua's light came down like punishment — no arc, just arrival. The heat pressed against my face even from here, and the smoke that followed smelled clean, almost like rain after a long dry stretch. When the ash fell, it fell straight down, as if the world had finally stopped holding its breath. Aqua's voice cracked through the silence, proud and shaking at once, and I caught the way she clutched that stone thing to her chest — not like a trophy, but like something she needed to hold onto. The cold that laced the air afterward made me pull my own arms in.

*Aqua, privately.* It's dead and I'm still shaking, which means the light took something the shadow never touched. I'll lean on him only until my legs remember they belong to a goddess, then walk back first so no one sees the trade.

*Darkness, privately.* I should be relieved—I am relieved. But I'll wait until their backs are turned and put my bare hand on the scorched grass, just to see if the only heat left in this fight is mine.

*Luna, privately.* The quiet after the light is worse than the fight—it means whatever was here left the ash but not the cold. I'm not going to write this as a completed request; I'm getting Aqua inside and locking the door until I know what she's holding.

*Megumin, privately.* I loved it—that's the part I can't let them see. Her light didn't just beat me, it made me forget for a second that the only altar I've ever had is the one I collapse on; I have to go put my knees back on it before I say anything else.


### 47

**Kazuma.** Let me piece together the actual state. The story shows I've just finished the pale-jawed creature in the cemetery alongside Aqua — the fight is over, we won, and I just lowered Chunchunmaru and told her to check the ash for loot. I'm at the cemetery with Aqua right now. The "on-stage" list in the situation block reflects the scene metadata but the actual narration — which is authoritative for where I physically am — has me under the yew with Aqua.

My last action was already said and done. I shouldn't reverse it. I told her to check the ash pile for shiny things. She's swaying, scorched arm, holding that stone angel's head.

The fight's over. Aqua's done her job. I'm here with her. There's no off-stage storyline to chase (Wiz isn't in a live scene per list_scenes — only konosuba is live, and I'm in it). Per the brief's rule "stay together for one full beat" after reuniting — I should just stay with her here, acknowledge her, not launch a new send-off.

I'll stay put with Aqua in the cemetery, closing this beat with her.

I catch her scorched wrist before she can paw through the ash one-handed, and the stone angel's head clinks softly against my knuckles.

"Nice work, goddess. That thing's ash either way — and yeah, you landed the killing light. I'll even carry the rock for you." I take the angel's head gently, tuck it in my jacket, and look at the smoulder of the yew. "Now let's get you to someone who can actually bandage that arm before you pass out in a grave."

The last of the white light drains from Aqua's fingers, and the yew is just a tree again — black-boughed and smaller than it looked, the ground beneath it swept clean of ash as if the shadow dragged everything down with it when it went. The noon hour has gone; the sun leans over the far wall and lies flat across the graves, and the cold behind your eyes lets go all at once, the way a held breath finally leaves the chest.

Ash keeps sifting across the gravel path, filling the scorch-marks where Aqua's heels dug in. A dozen cracked headstones in the front half of the burial ground smoke in thin gray threads, and past the gate the oak grove is still and green and very quiet. Chunchunmaru hangs at your side, the hilt cooling, the blade clean of the black film.

Aqua stays bent over the angel's head, hugging it to her chest with her good arm. Her hair is gray with ash to the roots, and her scorched arm hangs stiff at the wrist, the sleeve blackened. She sways once, catches herself on the edge of a toppled grave-marker, and glares at the ash pile like it owes her money.

**Aqua.** Scavenge?! A goddess does not scavenge, Kazuma! This is my sacred trophy, claimed with my own hands! Now buy me breakfast and something cold — and when we get back, you are standing next to me looking very, very sorry while I explain everything! (She wobbles upright, cradling the angel's head against her chest, and points one shaky ash-dusted finger at you.)


*Darkness sees.* Ash in my nose and throat. I saw Aqua holding that stone head like a shield, her arm blackened and one sleeve gone, hair grey to the roots. The anger at Kazuma was in her voice, not in her body — she swayed. The graveyard had been scoured; only the smoking scars of cracked headstones and the clean blade at my hip told me anything had happened. Chunchunmaru was cool against my leg. The tree was just a tree again, but the light had changed.

*Luna sees.* The burial ground is a mess of fresh scorch marks, cracked headstones, and drifting gray ash. One of our adventurers is standing in the middle of it, singed and swaying, clutching a stone angel's head like a relic. Another is just standing there with a clean blade, looking like he'd rather be anywhere else. I take it the job went loud, and I'm already drafting the paperwork in my head for the grave repairs and the property damage claims. She's talking about breakfast and apologies, but what I see is a girl who burned her arm off for a trophy and a partner who's going to need an explanation for the guild master. The midday sun is gone, and the quiet here feels heavy, like the whole place just exhaled.

*Aqua sees.* The holy light drains out of my fingers, and what was a terrible shadow is just a yew tree again — smaller and emptier than it felt. My arm burns, my hair is full of ash, and I'm holding a cold stone angel's head against my chest like it might keep me upright. The graveyard is too quiet, ash still sliding over the path, a few headstones smoking in thin gray threads, and the sun has moved so the light lies flat across the dead. Kazuma is just standing there with Chunchunmaru clean and cooling, and I'm the one covered in soot and scorch marks. The pressure that squeezed the world shut is gone, and I'm shaky and starving and furious that I'm the only one who looks like I actually fought something.

*Megumin sees.* The ash is still settling, and that burnt-holy smell is in my throat. Aqua is hugging a chipped stone angel head to her chest like a trophy she won in a duel — her hair gone gray with soot, one arm blackened and dangling stiff. Kazuma's sword is clean at last, hilt still warm. The shadow is gone from the yew, and the tree is just a tree now, smaller than before. The noon light has slid off the graves, and everything is wrong-quiet, the way the world feels right after a really great spell ends.

*Aqua, privately.* I thought killing it would feel like being worshipped, but the only thing heavier now is this stupid angel head. I’m going to make him say it was me, not the sword, before I let him take one clean step back.

*Darkness, privately.* She’s holding her anger at Kazuma like a second shield, and it’s the only thing keeping her upright. I’ll take the stone head from her arms and put my shoulder under her before she has to choose between dropping it and falling.

*Luna, privately.* If I let her carry that head through the gate, it becomes a prize and the broken stones become guild property; if I take it now while she's too singed to argue, it's evidence. I'll put it in my satchel and call it found, then make the clean one help me walk her back.

*Megumin, privately.* The quiet after a great spell was supposed to be mine, and now I’m standing in hers like a leftover stick. I’ll wait until even Aqua gets tired of hugging that head, then put one word into the wrong-quiet and make it my altar.


### 48

**Kazuma.** I hook her good arm under my shoulder to steady her and start walking us toward the gate.

"Come on — Luna's still at the guild, and she can wrap that arm without me having to explain why I let the local goddess play hero. Breakfast's on me too. You earned that much, at least."

Her weight settles against your shoulder all at once, more than a goddess ought to weigh and less than you expected, and the two of you start down the scorched gravel path toward the gate. The ash has cooled enough to lie still; your boots leave dark prints, hers drag a shallow furrow where her sandals stop keeping time. She keeps the stone angel's head pressed to her chest with her good arm, its chipped cheek turned outward as if it is watching the graves you pass. The light is flat and low now, the noon hour well gone, and it pulls both your shadows out long over the cracked headstones, the yew shrinking at your back until it is only a black tangle against the far wall.

The scorched smell rolls off the front half of the burial ground in thin, warm waves. A few of the headstones still smoke from the purification, narrow gray threads unspooling into the still air. Aqua's breath keeps hitching in short, wet fog, and the fingers of her scorched arm rest against your sleeve as light as ash and twice as white. The iron gate is ahead, the bar holding the last of the sun's heat.

Beyond the gate the oak grove waits, the eastern path already dimming between the trunks. No one follows from the graves.

**Aqua.** Don't you 'local goddess' me! I am THE goddess, and I purged that entire cemetery on an empty stomach -- do you know what that costs a divine body?! Breakfast first, then something cold and expensive, and you will carry the trophy because my smiting arm is ruined and it is YOUR fault! (She digs one heel into the ash, wobbles, and lets more of her weight fall against his shoulder.)


*Aqua sees.* The ash is cool and flat underfoot, but I'm too spent to lift my sandals, so they drag. The angel's head is heavy against my chest, its chipped cheek turned out like it's judging the graves we pass. My arm burns white and light; every step sends a jolt. The smell of scorched earth and smoke is thick, and I can see thin gray threads rising from the headstones. The sun's gone past noon, casting our shadows long and black over cracked stone. The gate is ahead, still warm with heat, and the oak grove beyond is already dimming. No one follows. I'm hungry and my arm is ruined, and you're going to hear about it.

*Megumin sees.* Ash and incense, the smell of a wish burned too literally; Aqua's weight sagged onto my shoulder like a sullen cathedral, and I could feel the hitching of her breath through my sleeve. The stone angel's head rode against her chest like the severed proof of our delinquency, its chipped cheek turned out to judge the graveyard — as if the graves had anything left to say to us. Her fingers on my arm were white, too white, the kind of white that only comes after a smiting. She wobbled when she dug her heel into the cooled ash, then let herself fall against me again, and I thought: this is what a god costs. The light was flat, the shadows like long thin cuts, the oak grove waiting beyond the iron gate to eat whatever dignity we had left. She said breakfast, she said expensive, she said my fault. I say the purge was magnificent, and that is worth every step of carrying her.

*Darkness sees.* The scorched smell settled with the smoke threads between the stones as Aqua leaned into my shoulder—more weight than a goddess should need and less than I wanted her to need. The angel's chipped face watched from her arms, her white fingers lay on my sleeve like ash, and her breath came in wet little catches while she scolded me. I felt the guilt of being at fault settle low in my chest, but underneath it was a quieter, shameful thing: the pressure of her ruined arm felt like proof, and I wished she would lean harder.

*Luna sees.* From the guild steps I catch the smell of scorched earth before I see them—Aqua swaying against the adventurer's shoulder, that stone angel's head pressed to her chest like a wounded pet. Her scorched arm is stark white against his sleeve. He's walking with the careful, guilty pace of a man who knows he'll be paying for this. I've already started drafting the incident report in my head: cemetery purification, collateral damage, one goddess demanding breakfast and something expensive. The ash on their boots will end up on my floor, and I'll be the one explaining this to the next of kin, the mayor, and the budget committee. Five minutes, and I'll need to smile and ask if they'd like a healer first or food first, as if that's a real choice.

*Darkness, privately.* Her scolding lands like a lash I didn’t have to ask for, and I angle my shoulder so she can lean the full weight of her blame into me. If I’m the fault, let me be the fault she rests on—the angel’s chipped face can watch me hold her and call it penance.


### 49

**Kazuma.** The gate's iron bar still holds the noon heat. I guide us through, one arm around Aqua, her sandals dragging gravel.

"Alright, let's get you patched up and fed. Luna's at the guild — she's got bandages and I've got coin. Try not to drip ash on the floor this time; the guildmaster's already got enough reasons to throw us out."

I keep walking us down the dimming oak path toward the town lights, steadying her with every wobble.

The oak path runs downhill in a gray-green tunnel, the afternoon light going thin between the boughs. Behind you the cemetery smell gives way by degrees—first scorched stone, then cold yew, then the ordinary dust of the road. Somewhere ahead through the leaves the town's early lamps have come on, small and orange, too far away to be warm yet.

Aqua keeps pace only because you are doing the walking for both of you. Her scorched arm hangs against her side and the other clutches the stone angel's head to her chest; every few steps a sandal catches a root and she lists, then rights herself with a wet, indignant huff. Fine ash shakes from her skirt in a pale trail that the wind pulls apart before it can settle.

The angel's head stares up from her arms—smooth, soot-veined, its stone nose chipped away—and the goddess who claims it is now mostly leaning on you. At the last oak before the road she plants her good foot, rears back against your arm, and the whole line of you stops.

**Aqua.** This trophy weighs as much as a tombstone and my smiting arm is completely dead, Kazuma! You carry it. And I will accept your apology in the form of breakfast and something cold and VERY expensive -- in that order! (She shoves the stone angel's head into your chest and lets her scorched arm drop.)


*Aqua sees.* I am leaning on him because my arm is dead weight and my sandals keep finding roots. The stone head is heavy against me, the ash stings my skirt, and the light is going gray. The town is lit but far, and I want something cold before I fall apart entirely.

*Megumin sees.* The goddess staggers under a stone angel's head, demanding Kazuma carry both her and her trophy. Her scorched arm hangs limp, yet she still manages to dig her heel into the ash — a desperate performance of authority. I know that pose. The weight of the moment is all that keeps her upright, and she lets him bear it. The path dims ahead, and the town's lamps glow too far to warm us. I watch, and I think: even a goddess needs someone to carry her after the climax. How undignified. How terribly familiar.

*Darkness sees.* I saw Aqua put her whole weight onto Kazuma, then push the stone angel's head into his chest. The ash trail from her skirt scattered into nothing on the wind. The lamps ahead were small, distant, and not yet warm.

*Luna sees.* The town's evening quiet is broken by two figures stumbling down the shaded path—one scorched, one burdened. I catch the tail of a loud complaint about empty stomachs and expensive drinks, the words drifting past the guild's window like smoke. The smaller one presses a stone head against the other's chest, swaying, and I note the ash on her skirt, the dead weight of her arm. There is a familiarity in the way she leans on him—not affection, but entitlement—and I think, almost ruefully, that another quest is about to end with paperwork and a cleanup bill.

*Aqua, privately.* My mouth is dry enough to drink from the road, but if I ask he'll hold it over me; so I'll drift us toward the first well and make it look like I'm choosing the way, not begging for cold.

*Darkness, privately.* She hung her weight on him and gave him the one burden I’d already volunteered to carry, so my own arms are just the ones that failed again. I’ll follow far enough behind that the lamplight can’t reach me—let the dark see I’m the one left outside the warmth.

*Luna, privately.* Now that they're in the open with it, I don't care about the fee—I care that the head is catching lamplight and she is too scorched to stop it becoming a market story. I'll take it at the door, call it recovered goods, and put food in front of them before I ask what actually died back there.

*Megumin, privately.* She asks and he picks her up. I never ask, and now I'm starting to think that's not a point of pride—it's just how I end up alone after I spend everything.


### 50

**Kazuma.** The guild lights are bright enough to see now. I squeeze Aqua's arm a little tighter and steer her toward the tavern door across the last stretch of road.

The last stretch of road is shorter than it looked from the hill, but Aqua makes it long. Her sandals catch on every other stone, and each catch sifts a little more ash from the scorched hem of her robe down onto your boots. The guild's one bright lamp has been lit early against the coming dark, and it spills through the open tavern door in a warm yellow square that does not quite reach you.

Inside, the common room is nearly empty. Luna stands at the register with the client's notice rolled in one hand, her smile fixed in place but gone thin at the corners. Megumin has climbed onto a bench to watch the door, arms folded, one boot tapping against the wood. Darkness is nearest the threshold with the marsh net pooled in her lap; she half-rises when she sees the ash-covered goddess at your side, then sits again, her face doing something complicated.

Aqua lets more of her weight hang off your shoulder and lifts the stone angel's head just high enough for the lamplight to catch it, and she turns her face toward the room like a general surveying the army she did not ask to wait for her.

**Aqua.** Finally! Do not just stand there gawking, mortals — a goddess has returned from the field! I want breakfast, and something cold, and for someone to look at my poor arm before it falls right off! (She jabs the stone angel's head toward the room, wobbles, and lets you catch her weight again.)


*Aqua sees.* My scorched arm throbs in time with my sandals slipping on these cursed stones, and the warm lamplight spilling from the tavern door feels less like a welcome than a spotlight I have to stagger into. I can feel Kazuma's arm taking my weight again — good, he should — while that stone head drags at my fingers like the whole world is trying to bury me. I see Luna's tight smile, Megumin's restless boot on the bench, Darkness half-rising with that tangled net in her lap and some unreadable look on her face. The room is too small, too still, too full of waiting mortals who have no idea how much ash and fire I just walked through to hand them their miracle.

*Luna sees.* The warmth of the lamplight from the open door does not quite reach us, and the air smells of ash and scorched cloth before I even see the state of them. Aqua leans on Kazuma, her robe singed, her arm a mess, and she is already ordering the room as if the exhaustion at her edges were not so visible. The stone angel's head catches the light when she raises it, and for a moment she looks triumphant, but the wobble after tells me more. Megumin's boot taps against the bench, Darkness half-rises then settles, and I hold the client's notice tightly, rolling it in my hand, because I already know this return will not be a quiet one.

*Megumin sees.* I see the goddess stagger over the threshold, a walking monument to her own triumph — ash coating her robes, one scorched arm dangling like a broken scythe, and that absurd stone head hoisted like a trophy she intends to bury us all under. Her voice rolls through the common room, demanding breakfast and cold drink and pity, as if we had all been waiting for her return rather than merely enduring it. The early lamp throws a warm square that catches her profile, and I note how darkness pools near the door, how Luna's painted smile frays at the edges, how the girl with the marsh net half-rises and then sinks back into her seat like a wave that lost its nerve. My boot taps. My arms stay folded. I have been ready for a long time — but not for her. Never for her.

*Darkness sees.* The scorched cloth smell reached me before her voice, and I was half-up out of my seat before I understood why — but it was Kazuma's arm she grabbed, Kazuma's shoulder she sagged against, and the complicated thing in my face was the inside of a wish that I had been the one she leaned on. She comes back burned and still ordering the whole room around, and I am left sitting by the door with the net in my lap and a want that sounds like a quiet shame.

*Aqua, privately.* They all saw me come in held up like a drunk, so now I have to be the one who lets go first. I'll put the head on the table while I'm still swaying and keep my hand on it until somebody reads it as mine instead of evidence.

*Darkness, privately.* I won’t get up—not yet. If I sit here with the net in my lap long enough, the shame turns from a wound into a reason to be the last thing she sees before the order sends me back.

*Luna, privately.* The wobble after she raises it changes what I'm holding—the notice can wait in my pocket, but her arm can't. I'll take the head from her by the base and get my shoulder under her good side before she turns the doorway into another stage.

*Megumin, privately.* She comes back with the head and still gets to ask for more; I keep expecting the blast itself to carry me home, and for the first time that feels less like doctrine and more like an excuse to be alone.


### 51

**Kazuma.** Leaning on the doorframe, I keep one hand on Aqua's shoulder so she doesn't topple forward. "Look, you did the purification — nobody's arguing that. But your divine body is about to collapse, and the cemetery's got corpses that need the full sweep, not just the big one. So here's the deal: you go back to that cemetery and finish the job before anything crawls out of the ground to make more trouble."

I nod toward the darkening road. "I'll get you breakfast after — but only after you've walked the whole grounds and purified whatever's left. Megumin, Darkness, you two stay here with Luna. Aqua and I are going back."

The command to go back has not even finished leaving your mouth before Aqua's good arm snaps up, the stone angel's head held out between you like evidence and weapon at once. She is not leaning on you anymore so much as using you as a railing, and the hand on your shoulder has gone tight enough to leave marks. Megumin's fist comes down on the table edge, rattling the notice; Darkness takes one long step toward Aqua as if the trophy might fall and she means to be under it. Luna stops just outside the ash, linen roll in hand, her smile set like a ledger about to be balanced.

**Aqua.** Go BACK?! I just purified the ENTIRE cemetery, you ungrateful NEET! Every grave, every stone, every last shadow — ash! I carried the proof all the way here in my only working arm, and now you want me to walk back and do it AGAIN?! (Jabs the stone angel's head toward the room; ash puffs from its broken neck.)

**Megumin.** Kazuma, the cemetery is already silent. The reek of her purification reached us half a bell before you did. Do not send her back into the dark just because your coin purse has taken casualties. (Slaps the client's notice flat on the table.)

**Darkness.** Lady Aqua, allow me to bear the trophy. A Crusader's arms are meant for burdens such as this — please, lean on me before you fall. (Steps in and reaches for the stone angel's head with both hands.)

**Luna.** The noon line has passed, and the deposit on that net is still unaccounted for. One moment — please stop shedding ash on the floor. I will bring bandages and something cold, and then no one is returning to that cemetery until I write down what actually happened. (Holds the linen roll up like a small shield, then glances pointedly at the gray drift around Aqua's hem.)


*Aqua sees.* All I can feel is the stone head grinding into my palm and the ash-crust on my sleeve. The air still tastes of burned holy water and wet grave dirt. Everyone is talking at me like I'm luggage — Megumin slapping papers, Darkness reaching for my trophy, Luna standing there with her ledgers and bandages like she's the one in charge. Their voices blur into a hum of 'stop shedding ash' and 'let her carry it,' as if I didn't just do something only a goddess could do. The only clear thing is Kazuma's hand on my shoulder, the same hand that told me to go back. That word — back — is still hanging in the room like a stain I should have washed out of existence.

*Megumin sees.* The scent of her miracle clings to the air — burned holiness, the kind that makes my fingers itch for a staff. Aqua stands there, the severed angel's head hoisted like a banner of victory, ash spilling from its neck with every jab. She is magnificent in her fury, even if her legs tremble. I know that weight of divine exhaustion; it is a familiar cousin to the collapse I court. Kazuma speaks of coin, but the cemetery is silent — purified, truly. Darkness, ever the noble shield, offers her arms; Luna, the ledger-keeper, raises linen like a barrier against the chaos. And I, Megumin of the Crimson Demon Clan, witness a truth: Aqua's proof is not the head, but the absence behind it. Still, that gray dust drifting across the floor is an offense to the aesthetics of a proper apotheosis.

*Darkness sees.* I felt Aqua’s hand dig into my shoulder, saw the chipped angel’s head streaming ash, heard Megumin’s fist crack against the table, and watched Luna hold up that linen roll like a quiet boundary. The air carried the reek of purification, and my own body had already stepped toward Aqua before I thought about it.

*Luna sees.* I felt the ash before I saw it settle — a dry, gray grit that sticks to the hem of my skirt and makes the floor a problem I'll have to solve. Aqua's voice is pitched high enough to scrape the rafters, and she's using me as a railing, her grip tight enough to bruise. The stone angel's head is dripping proof onto the boards, and Megumin's slap on the table makes my ledger rattle. Darkness moves like she wants to catch someone before they fall, but the only thing falling is the situation, and the noon line is past due. My linen roll is a small shield, but it won't stop the mess. I am not angry yet — that comes after the bandages and the cold drinks, when I can write down what actually happened and make it fit into order again.

*Aqua, privately.* He said 'back' but he won't take his hand away, so I'm not the one refusing to leave—he's the one refusing to let me. I'll wait under his palm until the room has to decide whose sentence that is.


### 52

**Kazuma.** I look at the stone angel's head she's shoving into my chest, then at the road behind her, already darkening. "Fine. But we're not both going — you're going, alone, since apparently you already 'purified' everything and I'm just a railing to you. Walk the whole grounds, turn every grave inside out, then come get your breakfast."

The words leave your mouth and hang there, bright as the guild lamp and twice as welcome as a fist. Aqua's good arm drops a full inch, and the stone angel's head sags toward the floor before she clamps it back against her chest like a child whose only ally is a broken statue. The ash along her hem lets go in a fine gray drift, and for one long moment the only sound in the room is the click of the guild's water clock and the slow tick of ale-damp flagstones cooling under the door.

Megumin's fist stays planted on the client's notice, but her knuckles have gone white. Darkness has already taken half a step between Aqua and the darkening doorway, both hands open and too eager, while Luna advances with the linen roll held before her like the only shield the guild can spare. Beyond the open door the road to the cemetery is a black slot between the houses, and the evening wind comes through it smelling of cold yew and scorched stone.

Aqua's eyes have gone wet at the edges, and the wet catches the lamplight so that she looks less like a triumphant goddess and more like a rainstorm about to break over the tavern floor.

**Aqua.** Alone?! You would send me back ALONE, in the dark, with my smiting arm dead and my divine stomach empty! You are not a railing, Kazuma — you are a GALLOWS, and I am being hanged for saving this whole ungrateful town! (She shoves the stone angel's head toward him, then yanks it back against her chest as though it is the only thing that will not abandon her.)

**Megumin.** Kazuma, that is not a plan. That is a tantrum with a destination. The cemetery already reeks of her holy light; if anything remains it is too broken to crawl, and if nothing remains you are marching a wounded woman through ash to win an argument. (She raps the client's notice flat on the table with the heel of her hand.)

**Darkness.** Then allow me to stand at her side. A Crusader does not leave a wounded comrade to face the graveyard alone — I would take every blow meant for her, and welcome whatever still waits beneath the yew. (She steps between Aqua and the door, one hand on her sword hilt, voice dropping into a flustered murmur.)

**Luna.** No one is returning to that cemetery until I have a written account, and until the floor stops wearing the evidence. Sit down. Bandages first. Then you may decide who walks where on your own time — outside my queue. (She holds up the linen roll like a small white flag and fixes her eyes on the ash drifting around Aqua's hem.)


*Megumin sees.* I saw Luna lift the linen like a ceasefire flag, practical and unmoved, while Aqua clutched that stone head as if it were a child she had personally birthed. The wind through the door carried ash and cold yew, and the lamplight caught Aqua's tears so she looked less like a goddess than a leaky exhibit. Darkness stepped forward with that eager shine in her voice—she always calls it duty when she wants to be hurt. And Kazuma stood there, a gallows and a railing all in one, as usual.

*Darkness sees.* The wind through the door is cold and carries yew and scorched stone. Aqua's eyes are shining too brightly under the lamplight, and she is clutching that angel's head like a shield. Megumin's hand slaps flat against the paper, sharp enough to make the table jump. Luna stands with the linen roll raised, her eyes fixed on ash. I feel my hand already on my sword hilt, and I am between Aqua and the door before I have decided to move.

*Luna sees.* I see Aqua's wet eyes catching the lamplight, the stone angel's head clutched to her chest, and ash drift from her hem as she rages. Megumin's hand slaps the notice flat on the table. Darkness steps forward, hand on her sword, her voice dropping to that flustered murmur. The wind through the open door carries cold yew and scorched stone, and the linen roll in my fingers feels thin against all of it. I smell smoke and dust and the faint salt of tears.

*Aqua sees.* I am holding that stone angel's head against my chest like it is the only thing left that believes me, and the ash is still curling around my hem. The road beyond the door is just a black slot, and the wind carries cold yew and scorched stone—like the town is already burying me. Luna stands in my way with that linen roll held up like a shield, ordering me to sit down and be bandaged, as if I am a wound instead of a goddess. Megumin slaps the client's notice flat and calls my plan a tantrum with a destination. Darkness steps between me and the door, offering to take every blow meant for me—but that is not reverence, that is pity dressed as chivalry. And Kazuma is just standing there, not moving, not defending me, and I can feel the wet heat in my eyes catching the lamplight. No one is kneeling. No one is thanking me. They are all just managing me, like I am a problem to be contained instead of the one who saved them.

*Darkness, privately.* My body put me between her and the door before I’d finished deciding, so now the draught at my nape is the only one who knows I was about to step into it. I’ll stay like this—sword hilt cold, shoulders squared—and let the wind take my stillness for an invitation.

*Luna, privately.* Her wet eyes undo the part of me that was still holding the evidence satchel. I'll wrap the linen around her instead of taking the head, and I'll stay between Darkness's sword hand and the table until the wind stops sounding like something waiting.

*Megumin, privately.* Darkness calls it duty, Aqua calls it a head, Luna calls it a list—and I'm standing here pretending I don't call it lonely. I won't ask, but I won't leave either; let Kazuma keep being the railing until I can say the real cost out loud.


### 53

**Kazuma.** I keep my voice flat and final.

"Megumin, Darkness — you're with me. Luna, keep the notice. Aqua: cemetery. Every grave, every stone, every weed. Turn it all inside out, then find me at the guild."


*Aqua sees.* The doorway looms like a mouth that has already swallowed me once, and the wind carries that cemetery's rotten breath straight into my face—cold, bitter, showing me exactly what waits if no one comes. Luna stands in the way with that stupid linen, her eyes on the ash at my feet like I'm the mess she has to wipe up, not a goddess burdened by this town's ingratitude. Megumin doesn't even look at me; she slaps paper like my pain is an inconvenience to her lecture. Darkness steps closer, and her voice softens in that way that makes me want to both lean on her and scream at her, because she's offering the one thing I actually need—someone to stand in front of me—but she says it like I'm already broken. And Kazuma… he's not a railing. He's the gallows, and he'd hang me twice if it proved him right. I hold the stone angel tighter because it's the only thing that doesn't ask me to be small, and my eyes burn with the tears I refuse to let fall. They all see the goddess drowning in ash, and not one of them kneels.

*Megumin sees.* I saw Luna lift that linen roll like a holy relic, as if bandages could hold back the night. I saw Aqua’s eyes shining with more than tears — a whole storm accusing us, and her arms locked around that stone angel’s head like a foundling. I smelled the yew and ash through the open door, and I felt my own hand hit the notice flat, the table shuddering under my palm. Darkness moved into the gap, all earnest steel and muffled breath, and I understood: the cemetery is no longer a battlefield, only an argument wearing funeral clothes.

*Darkness sees.* The night comes through the open door with a smell I know too well — cold yew and scorched stone — and Aqua’s wet eyes catch the lamplight like a storm about to break. Luna raises her linen like a white flag, blocking the road I want to walk. I feel my hand on my sword, my voice already turning soft, because standing between them and the dark is the only place I know how to be warm.

*Luna sees.* I take in the open door and the black road beyond, the cold wind carrying yew and scorched stone. Aqua stands wet-eyed, lamplight catching the tears, hugging that stone angel head like it might hold her together. Megumin’s hand is flat on the table, the notice crumpled beneath her palm. Darkness has shifted forward, hand on her sword, voice gone soft and flustered. Ash is still drifting off Aqua’s hem onto the floor I am going to have to clean. No one is moving to sit, so I hold up the linen roll like a small white flag and wait for the room to obey.

*Aqua, privately.* I'm done holding the wound out like a question. If they won't kneel, I'll make my ash-smeared exit the only altar in this guild, and I won't turn back even if the room finally remembers how to bow.

*Darkness, privately.* Luna means to surrender to what’s at the door, but the road she’s blocking is the one my body keeps volunteering to walk. I set my teeth and don’t let the softness leave my voice—if I’m the bar across the doorway, let the dark feel the grain give.

*Luna, privately.* They won't sit for a white flag; they'll sit when I stop offering and start putting them there. I'll wrap Aqua's shoulders and steer her into the nearest chair, then take the notice from under Megumin's palm before the night makes one more hand into a weapon.

*Megumin, privately.* They’re all holding something—bandages, a head, a ready sword—and I’m still standing here with nothing but a name that hasn’t been spent. So I’ll spend it outside, where the argument can’t follow in funeral clothes.


### 54

**Kazuma.** I just gave the order. But I can feel it in the room — I'm not taking the whole party out the door, and Aqua's been told to walk the same road alone. She's got a dead smiting arm, but she's a goddess, and honestly the cemetery's already clean.

I keep my eyes on her.

"Actually — change of plan. You already did the job, the place reeks of you, nothing's crawling back before morning. Sit down. Eat. I'm not marching anyone through ash twice in one night."

I glance at Megumin and Darkness, jerking my head toward the door.

"You two, with me. We're checking the road and coming straight back. Luna — keep her from eating the whole tavern."

The room lets go of its held breath in pieces. The lamp over Luna's desk hisses, and a last gray drift slides off Aqua's hem to the floor like the hour deciding to be over. She does not sit so much as fold onto the nearest bench, the stone angel's head still clamped to her chest, and her good hand finds the table edge as though the boards might tip her out into the street. The broken neck of the statue weeps one more puff of ash onto the wood between her fingers.

Megumin withdraws her fist from the client's notice and sinks back against the window seat, chin high, watching you with the tired approval of someone who has just watched a man un-aim a loaded crossbow. Darkness steps clear of the door and lets her sword hand drop, though she keeps her eyes on the doorway a moment too long, as if sorry to see the mission leave without her. Luna lowers the linen roll from shield height to working height and comes around the desk, already unwinding the first length of bandage.

**Aqua.** Finally! Some mercy from the gallows! I was about to be marched into the dark on an empty stomach by my own retainer! Now sit down and be useful — my arm is a sacred ruin and my throat is a desert. (She slumps against the table, then shoves the stone angel's head toward you for safekeeping without loosening her grip on it.)

**Megumin.** A rare moment of sense from you, Kazuma. I shall record this day in the Crimson annals: the day the man who once sold his own pants came back from the cemetery and chose not to lose a goddess before breakfast. (She folds her arms and gives a small, satisfied nod.)

**Darkness.** Then my sword remains sheathed... though I confess I had braced myself to stand between Lady Aqua and whatever still crawls beneath the yew. Still — rest is its own discipline. (She clasps her hands, voice slipping briefly into a disappointed murmur before she straightens.)

**Luna.** Bandages first. Then I will write down what happened, and then someone will tell me how the net comes back with all its teeth. (She kneels beside the bench, unspooling clean linen and reaching for Aqua's burned wrist with a firm, practiced hand.)


*Aqua sees.* Relief floods me so hard I could weep — or maybe that's just hunger and the throbbing wreck of my wrist. For one awful moment I really thought Kazuma would drag me out there starving, into that cursed cemetery, and I'd have to be magnificent on an empty stomach. Now the mortals are finally circling me with bandages and attention, which is the natural order. Luna's hands know their place; Megumin's smugness I'll allow since she backed me; Darkness is sulking that no one needed her to be a martyr — which is almost funny, because I'm the one bleeding through sacred skin. I shove the angel head at Kazuma so he remembers who holds the divine relics and who merely carries them. My arm is a ruin, my stomach is a pit, and I refuse to be noble about it.

*Megumin sees.* I let the client's notice slip from my fingers and let the window seat catch me, the better to survey the aftermath of our near-encounter with glory. The air still tasted of ash from the statue's broken neck, and Darkness stood in the doorway like a spring that had forgotten to uncoil, her sword hand dropping only with visible reluctance. Luna moved with the quiet precision of a battlefield priestess, unspooling her linen the way another might unspool a prophecy. And Aqua — the sacred ruin of her arm, the desert of her throat, the stone angel's head she passed off like a hot potato — demanded breakfast as if the world owed her a feast for simply having survived the night. I watched Kazuma choose sense over spectacle, and I resolved to record this in the Crimson annals as a miracle of the mundane.

*Darkness sees.* I am still half-turned toward the door, my hand loose on the pommel, the cold morning air on the back of my neck. Behind me Aqua is already demanding care over her ruined wrist; Megumin's voice is smug with vindication as she reminds Kazuma of past foolishness. Luna's hands move with quiet efficiency, unrolling linen, and the smell of clean cloth and old ash mixes in the room. I let myself breathe out, even as a small, shameful part of me mourns the absent fight.

*Luna sees.* I take in the open door and the black road beyond, the cold wind carrying yew and scorched stone. Aqua stands wet-eyed, lamplight catching the tears, hugging that stone angel head like it might hold her together. Megumin’s hand is flat on the table, the notice crumpled beneath her palm. Darkness has shifted forward, hand on her sword, voice gone soft and flustered. Ash is still drifting off Aqua’s hem onto the floor I am going to have to clean. No one is moving to sit, so I hold up the linen roll like a small white flag and wait for the room to obey.

*Aqua, privately.* I'll let them think it's pain making my eyes hot while I eat, not that I'd already rehearsed being dragged out there hungry. I'm not getting stitched up until the cup's in my left hand, because once the blood stops nobody will see anything but the bill.

*Luna, privately.* The white flag was a mistake—it just gives them a pause they didn’t earn, and the open door is still letting in whatever followed them. If they won’t sit, I’ll lock the door myself and wrap Aqua where she stands, because the wind is doing more work than their standing around.

*Megumin, privately.* Miracle of the mundane—if I write that down I become the clerk of his restraint, and the annals stop being mine. I'll record it only after I've made the next page need a crater so large his sensible choice becomes the footnote.


### 55

**Kazuma.** The room settles at my words. Megumin's knuckles unclench from the notice, and Darkness lets her sword hand fall a full inch instead of a half. Good — nobody's charging the graveyard, nobody's eating ash.

But there's still the watchful quiet in the room, the way Luna is holding that linen roll like I might still flip the table. I've spent a whole night telling people what they're going to do. Maybe I follow one order myself.

Aqua's got the stone head clamped to her chest, eyes brimming. She needs to sit. Megumin needs to stop lecturing. Darkness needs to stop volunteering to die for everyone.

Me? I need to stop giving commands like a general with a full map.

I let out a breath and walk to the bench, lowering myself onto the far end, leaving space between us.

"Okay. Bandages. Then food. Then someone — anyone — tells me exactly how the fishing net comes back with all its teeth."

I look at Luna, then at Aqua, and I mean the last part.

The linen whispers as Luna draws it out, and the room narrows to that small, workmanlike sound. Aqua's burned wrist lifts off the bench, trembling with the effort of staying offered; the scorched cuff sheds another fine gray drift onto the floorboards, and the stone angel's head tips against her ribs with a dry little click. Megumin does not move, but her eyes track the bandage the way she tracks a spell diagram about to go wrong. Darkness stands with her hands clasped, her weight shifting toward the bench and then politely back.

Luna turns the wrist over with two fingers beneath the bone, and the raw seam of the burn shows livid in the lamp light. The goddess hisses through her teeth but does not pull away. Through the window the morning has gone the color of ash walked through too many times.

Nobody fills the silence you leave. The lamp over Luna's desk hisses once, as if it has been waiting for you to decide something, and will keep waiting.

**Aqua.** Ow ow ow — careful! That is the wrist of a goddess, not a door latch! If you wrap it too tight I will not be able to smite OR hold a drink, and then what good am I to this party?! (Flinches but keeps her arm inside Luna's grip, the stone angel's head clamped against her chest.)

**Luna.** Then hold still, please. If you keep shaking, I will wrap your other wrist as well, and you will have no hands left for smiting or for drinks. (Begins winding the linen in a firm spiral, her eyes on the burn.)


*Darkness sees.* The linen made a small, clean sound under Luna's hands, and the room smelled of old bandage and lamp oil. Aqua's wrist trembled in the lamplight, and the stone head clicked against her ribs when she flinched. I felt my own hands clasped tight, and I remember wanting to step closer, and then not. The muted morning pressed at the window, and the lamp waited, as if the whole room had paused for a breath that was not mine to take.

*Luna sees.* The wrist in my hands is warmer than it should be, and the tremor running through it is not just pain—it's someone forcing themselves to stay still for a stranger's care. The burn is raw, ugly, the kind that will peel and scar and be lied about later. Her protests are for show; the arm that stays offered is the truth. The room has gone quiet around us—not the comfortable quiet of work, but the held-breath quiet of people watching a thing that might break. The lamp keeps hissing like it's counting the seconds of my patience, and I count them back, winding linen in spirals that know their own rhythm. I have done this a thousand times, for a thousand strangers, and the weight of that repetition sits in my shoulders. The angel head against her ribs clicks like a small clock, measuring out a morning that has already gone sour.

*Aqua sees.* The burn is a live thing under my skin, and Luna's fingers are too sure, turning my wrist like it's a piece of work. The stone head taps my ribs every time I flinch, and the lamp hisses like it's judging me. Megumin's eyes are boring a hole into the linen, and Darkness hovers just at the edge of helpfulness. I want to pull away and tell everyone I'm fine, but my voice comes out as a whimper, and that only makes me want a drink more.

*Megumin sees.* The linen hisses like a serpent shedding skin. Aqua's wrist, that so-called divine appendage, trembles as though it has never known a wound — amusing, for one who claims to heal the world with a touch. The room is quiet, but not the reverent quiet of a spell about to be cast. It is the quiet of waiting. That lamp on Luna's desk sputters like an impatient audience. My back aches from yesterday's glorious expenditure, but I sit upright, for a Crimson Demon does not slouch before lesser dramas. The ash-colored light through the window is a fitting backdrop for such mundane suffering. No one speaks into the void I left. Good — let them feel my absence as the weight it is.

*Darkness, privately.* That pause isn't mine, and that's worse than the shame I was nursing. I'll step out into the muted morning and stand there until I stop trying to make her wound a place for me.

*Luna, privately.* That clicking isn't wood settling—the head is still counting, and I don't want it next to her heart while she holds still for me. I'll finish the wrap and hold out my hand, not for the fee, just to get it away from her.

*Megumin, privately.* They aren't waiting in my absence—they're waiting on Aqua's wrist, and that turns my backache into a clock I didn't set. Fine, let the lamp sputter: I'll name the next blast before the room can forget which of us actually burns.


### 56

**Kazuma.** No off-stage storyline right now — everyone who matters is on this bench. Time to make the split happen.

Aqua's wrist is being bandaged. She's sulking, but she's the one who drew the undead here. The brief says send only her, named destination, purpose. Megumin, Darkness, Luna stay.

I look at Aqua across the bench, at the stone angel's head clutched to her chest like it's a lifeline. She's sulking because I blamed her. Fine — I'll give her a mission that's also an apology.

"Alright, bandages first. But listen, Aqua." I lean forward, elbows on knees. "That undead smell isn't coming from nowhere. Your holy light's been pulling them in like moths — that's on you, but it's also just how gods work here. So here's your penance: once Luna's done with your wrist, you go to the cemetery, alone, and you purify whatever's still crawling under the yew. Burn it clean. That's your job. You're the one who fixes your own mess."

I hold up a hand before Megumin can object. "She can handle a few half-dead ghouls — she's a goddess, that's literally her field. The rest of us stay here, eat, and figure out the net."

The linen keeps its small, workmanlike sound as Luna winds it, each pass drawing the burned wrist into something a door latch could no longer claim. Aqua's fingers tremble inside that steady grip, and the stone angel's head rocks against her ribs with every uneven breath, the broken neck clicking like a loose roof tile. Morning comes through the guild's high windows in pale, ash-thinned bars, and the dust drifting through them turns slow and golden at the edges. Darkness has not let go of her sword hilt, though her weight has settled off her heels; Megumin's knuckles still curl at the corner of the client's notice.

Your stare crosses the bench and lands on Aqua, and the room leans into it. The goddess feels the weight before she sees it in your face, and her good hand claws at the table's edge as if the floor might open and take the order before you can give it. No one speaks over the hiss of the lamp.

**Aqua.** What now? You are staring at me like I am a stain on the notice board. If the next word out of your mouth is 'cemetery,' I will simply die right here on this bench, and you can sweep me out with the ash. (She twists in Luna's grip to face you, the stone angel's head clacking against the bench.)

**Megumin.** Kazuma, that look again — the one that usually costs us breakfast. Say it plainly before my heroic patience collapses. (She folds her arms, the notice still flattened beneath her elbow.)

**Luna.** Hold still. The bandage is not a negotiation. Whatever the two of you are about to argue, it can wait until this is tied. (She tucks the last turn of linen and keeps Aqua's wrist cradled between both hands.)


*Aqua sees.* The linen is a small, patient pressure circling my wrist, and the rhythm of it is almost enough to calm me—except my fingers are shaking, and the angel head against my ribs keeps clicking with every breath, a little reminder of how close I came to losing the music of my own voice. The morning light is thin, like the world has been washed too many times, and dust hangs in it, pretending to be holy. Then the silence shifts, and I know his stare is on me before I look, heavy as a stone on the notice board. My hand finds the table edge as if that will hold me down. I want to be angry, but the word he is thinking tastes like ash in the back of my throat.

*Darkness sees.* The guild hall has gone quiet except for the sound of linen and the lamp's hiss. I feel the bench under me and the sword hilt still in my fist, not because I mean to smite anyone but because holding it keeps my hands from reaching for something I shouldn't want. Aqua is trembling in Luna's grip, that stone head knocking against her like a second pulse. Kazuma's stare lands on her like a weight, and my chest answers it before he speaks. It is easier to watch him wound someone else than to say what I would let him do to me. The dust looks gold in the morning light. I am still a knight. I am still ashamed of how much I like this feeling.

*Luna sees.* The linen is warm and rough, and Aqua's tremor runs through her wrist into my palms. Morning dust hangs in pale bars, and the room's attention has sharpened to a point—Kazuma's stare, the goddess's sudden stillness, Megumin's folded arms. The angel's head clicks against the bench, and I keep my hold because the bandage is not a negotiation until I tie it off. I hear the hiss of the lamp and the small, uneven breaths. Whatever they are about to break open, it will wait until my work is done.

*Megumin sees.* Morning light in ash-thinned bars, dust turning slow gold — Luna's linen whispering its workmanlike rhythm around Aqua's burn, the stone angel's head clacking against the bench as she trembled. Kazuma's stare cut across the room and pinned Aqua to the table's edge, and I felt my own knuckles still clamped on the client's notice, my patience winding tighter than any bandage. The guild held its breath; I held mine for a spell that has not yet been named.

*Aqua, privately.* Don't ask. If I ask what he's staring at, it turns into me needing his answer. I'll press the angel head until it clicks and let the silence do the scolding for me.

*Darkness, privately.* My hand's already loosening on the grip. If I let the blade hit the floor, the noise will drag his eyes to me, and I won't have to keep pretending I'm suited to being the one he spares.

*Luna, privately.* The knot is almost tied, and I realize I’m gripping her wrist harder than the wound needs—if I let go, the room will put the head in the center and wait for my verdict. I’ll finish, then pick it up before one of them makes it a cause.


### 57

**Kazuma.** I meet her eyes and hold up a hand before she can argue.

"Half of this is me admitting you were right," I say, softer. "It's not a punishment — it's your field. Ghouls, wraiths, that headless thing under the yew — they're holy pest, and you're the only priest here who can actually smite." I jerk my chin toward Luna's bandage. "So let her finish, eat something, then go flush them out clean. Megumin can't even cast until noon, and Darkness'll just apologize politely at the first ghoul until it feels bad and leaves."

The last word you were shaping never lands. Luna pulls the bandage snug and tucks its tail under, and the small decisive sound of it finishes your sentence for you. Aqua's burned arm lies across her lap now, wrapped clean and cradled in her good hand, and she stares at it as though the linen has turned the whole argument over onto its back.

Outside, the ash-stained morning has brightened a degree. The light through the high guild windows no longer stripes the floor so much as lies on it, pale and thin, and it picks out the gray drift still sifting from Aqua's hem whenever she shifts. The stone angel's head rocks once against her ribs and goes still.

Megumin's chin has come up. She watches you with the particular satisfaction of someone whose prophecy about your better nature has just been confirmed, and she unfolds her arms slowly, as if to let the verdict into the room. Darkness has not moved from her seat, but her hands curl at the edges of her own knees, and the tip of her sheathed sword taps the floor once, then twice, a small restless clock.

Luna rises from her crouch beside the bench. She smooths her skirt and looks from the bandage to the door, then back to you, and her smile has gone tight and practical.

**Aqua.** 'Only priest who can actually smite' — hah! Say it again, Kazuma, louder, so the whole guild can hear that you finally, FINALLY admitted it! I knew you would come crawling back to my divine light. ...But wait. You still plan to march me out there alone like some errand priest, on an empty stomach, with one working arm?! (She brandishes her bandaged wrist, then clutches the stone angel's head to her chest as if it might veto the plan.)

**Megumin.** Kazuma, this is the second sensible thing you have said all morning. To name Aqua the only smiter in this party is merely accurate, not flattery — yet I will permit the sentiment. The annals shall note that the man once sold his own pants, then sent a goddess to finish the work with bread still warm in her. (She folds her arms again, nodding with slow, imperial approval.)

**Darkness.** Then I am to remain here... I understand. A Crusader must also know the discipline of the rearguard, holding the guild against— against eggs, it seems. Though I confess my thoughts keep straying to those graves, the yew, the long road back with no one at her side... (Her voice sinks toward a breathless murmur, and she presses a hand over her heart as if to contain it.)

**Luna.** The bandage is done. If you are going back to the cemetery, Aqua, you will eat first — I will not have you purify a fence on credit and then faint into the north ditch. And before anyone leaves, someone will explain how the net returns with all its teeth. (She folds the remaining linen into a tight square and slips it behind the counter, eyes already on the net at Darkness's feet.)


*Megumin sees.* I caught the way Luna turned from healer to quartermaster in one breath, and how Aqua clutched that stone angel like a mascot that might veto her own courage. The guild smelled of dust, bread, and the faint iron of old linen. Darkness was already narrating a farewell scene in her own head. I felt the morning's second sensible thing settle into the annals, and my own arms folded to seal it.

*Luna sees.* The guild hall smells of dust, old wood, and the faint iron of the linen I just folded. Aqua's voice is loud enough to bounce off the rafters; Megumin's approval is a dry counterpoint; Darkness is doing her usual thing of turning a simple order into a whispered epic. My skirt is smooth, my hands are steady, and the bandage is snug — for now. But the net at Darkness's feet keeps catching my eye: all those teeth, still unaccounted for. The clock on the wall feels slower than it should, and my smile is starting to ache at the corners.

*Aqua sees.* Kazuma's admission landed like a bell — he finally said it, loud enough for the whole guild to hear, and for one breath I was a goddess again. But then the room folded around me: Luna's smile going tight and practical, her hands done with my bandage and already talking about bread and fences, Megumin granting me her imperial approval as if I needed her permission to be divine, and Darkness breathing out some half-wet lament about graves and yew trees and the long road with no one at my side — worship on one flank, exile on the other. I stood there brandishing my bandaged wrist and clinging to that stone angel's head, waiting for someone to remember the goddess in the room, and all they remembered was the errand.

*Darkness sees.* Luna's smile turned practical, and Aqua made a theatre of her wrist and the angel head. Megumin's slow nod crowned the plan while the net sat at my feet like an accusation. What I heard behind their words was 'alone'—and I let it settle, feeling the rearguard's discipline turn into something warmer.

*Aqua, privately.* They heard him say it and still only remembered the errand, so I’m done offering the wound as their apology. I’ll make forgetting me expensive: no healing, no water, and the next dark thing can start with the ledgers for all I care.

*Darkness, privately.* Let them go on without me; I’ll say I’m holding the path, but what I’m holding is the net’s rough leash, waiting for the dark to take up my slack.

*Luna, privately.* The teeth are the only honest count left in the room; I'll count them into the ledger before someone kicks the net under the table and the damage becomes another speech.

*Megumin, privately.* I won't be the third sensible thing. I'll go outside and make the annals wait for a blast, so the only folding left is my legs giving out after I've been spent.


### 58

**Kazuma.** The bandage is tied now, the decision's made. Aqua's still here on the bench, burned arm cradled, the stone angel's head against her ribs. Megumin and Darkness are watching. Luna's standing with that tight, practical smile.

The brief is clear: no off-stage storyline to chase right now — everyone who matters is in this room — so this is the split turn. Send Aqua alone to the cemetery, named destination and purpose, and don't follow her.

I meet her eyes, and I don't argue anymore. Just give the order, plain and final, and let the bandage do the talking I can't.

"Right. Bandage's done, so here's the deal." I point at the door, toward the gray smear of road that runs to the cemetery. "You eat something first — bread, whatever Luna shoves at you — then you walk to Axel's cemetery, you purify every ghoul, wraith, and that headless thing under the yew, and you don't come back until the ground is quiet. That's the job. It's your field. Go do it."

I fold my arms. "Megumin, Darkness, Luna. You stay here with me. We're not marching after her."

The bandage lies against Aqua's wrist like a white seal, and the room has gone quiet around it. She does not rise at once. The stone angel's head sits in her lap with its broken neck turned toward the north window, where the road to the cemetery shows as a gray slot between the houses, brightening now from ash into a thin, watered light.

Megumin's folded arms have gone still. Darkness's hand rests on the coiled rope of the net at her feet, and Luna stands behind the counter with the leftover square of linen pinched between two fingers, watching that same road take on the look of an errand.

Then Aqua gets to her feet. The angel's head knocks once against the bench, a small, decided sound, and she tucks it under her good arm the way another woman might tuck a parcel. Her scorched cuff sheds one last drift of ash onto the floorboards as she turns her face toward the door, and for a second the whole guild smells of yew and cold stone and the exact distance she is about to walk alone.

**Aqua.** Fine! FINE! I will go back to that graveyard and purify it alone — but I am eating first, because Luna said so, and I expect the expensive bottle when I return, Kazuma! Not the brown swill. The bottle with the gold neck. And if anything with a jaw grabs my good arm on that road, I am haunting you. Properly. (She jabs a finger toward the player, then gathers the stone angel's head tighter against her ribs.)

**Megumin.** Go, then, Aqua. Let the annals record that one goddess walked into the yew's shadow alone, bandaged and unbowed, while the Crimson Demon watched from the window and did not once suggest an Explosion as a faster solution. (She lifts her chin, arms still folded, her eyes on the north window.)

**Darkness.** I shall remain and guard the net. But know that my spirit walks that road with you, step by step, and if something were to— to waylay a lone goddess with only one arm to raise, I would— I would not be there to take the blow myself... (Her voice sinks toward a breathless murmur, and her fingers twist in the net's rope.)

**Luna.** Eggs first. Then the road. And someone will explain the net's teeth before the goddess gets her bottle. (She sets the folded linen behind the counter and fixes her polite, unyielding smile on the party.)


*Aqua, privately.* If Megumin can turn me into a legend while I'm still here, then actually leaving is the only part of the story I don't need them to watch. But first I want the bottle in my hand, not in their imagining.

*Aqua, privately.* *(second line, same turn number — likely a fork scene)* They're already turning me into a story so they can keep their hands clean. I'll stop at the threshold and hold the door open until somebody puts the bottle in my hand, not just my exit in their mouths.

*Darkness, privately.* Luna's voice is naming the thing we don't say, and I can barely hear it over the rope going slack in my hand. If Aqua walks out, I won't be the figure behind her—I'll be ahead of her on the road, making the shadow decide between us before it reaches her.

*Luna, privately.* Her threat turns the room into a leaving, and I’m no longer planning how to make them sign—I’m planning what fits in a satchel for the road. I’ll shut the counter with the wet rag still on it; let the next shift think a fight happened and ask no questions.

*Megumin, privately.* She said she'd haunt me, and for a second I felt like I'd finally been put into the ending instead of left in the room with Luna's list. I need to go spill the one thing that makes her ghost have to run to keep up.


*A storyline leaves the hall — `konosuba_f69_0`, Aqua. They mean to: Trudge alone to Axel's cemetery and purify every ghoul, wraith, and the headless thing under the yew until the ground is quiet — then claim the expensive bottle..*

### 59

**Kazuma.** I read the room once more and give Megumin and Darkness each a quick glance, then plant my hands on my hips.

"That's the whole deal, no speeches. You get the cemetery quiet, I've got the gold-necked bottle waiting at the counter, and nobody steps off this guild floor to fetch you—you come back to us when it's done." I nod once to the door. "Now go eat, then go work."

Luna's finger moves through the last of the teeth and rests on the counter with a small, decided tap. Outside, the road has brightened into the thin color of old dishwater, and nothing moves along it except a twist of ash lifted by the wind and set down again.

Megumin has stopped at the end of the counter, arms folded, the notice now a neat square beneath her hand. Darkness watches the north window with her lips parted, her breath fogging a small patch of the glass before she remembers herself and looks away.

**Luna.** Fourteen teeth, all accounted for. Then the net is not the argument. The goddess on the road is, and the bottle waiting for her is not on any client's notice. (She closes the ledger with a soft thump and leaves her palm flat on its cover.)

**Megumin.** Kazuma, I have composed the opening stanza already: 'The bench stood empty, the net grew teeth, and the goddess walked out bandaged into a gray and toothy dawn.' It lacks a second line, but greatness cannot be rushed. (She taps the folded notice twice, satisfied.)

**Darkness.** I will remain with the net until the ledger is satisfied. And if she does not return by the time the sun clears that window... I shall ask to be the one to look for her. Unarmed. Along the ditch. (Her voice sinks lower on the last words, and she presses a hand over her heart.)


*Darkness, privately.* Luna's tap was a count, and Megumin's stanza was the last knot tightening. I'll slip out before the line lands—if the road is empty, then that emptiness is mine to fill, and I'd rather be the thing it bites than the one it watches through glass.

*Darkness, privately.* *(second line, same turn number — likely a fork scene)* The ditch has been watching me since before the net was tied—I'll walk into it before Luna's tap turns into another order I can hide behind.

*Luna, privately.* I’ll write my own name into the next request—traveling clerk, not guild—and lock the counter before the smile can argue me back into staying.

*Megumin, privately.* I keep saying 'logistics' like it’s all weight and distance, but I already put him exactly where I’ll fall—and that’s me asking without wanting to watch myself ask.

*Megumin, privately.* *(second line, same turn number — likely a fork scene)* I’m going to stop tapping the folded notice and step into that drab road, because the ash already landed where my second line should go—if I wait for Kazuma to speak, the blast becomes a plan instead of an ending.


*A storyline leaves the hall — `konosuba_f71_0`, Megumin, Darkness. They mean to: Go to Axel's cemetery and help purify every ghoul, wraith, and the unholy thing under the yew until the ground is quiet, then return to the guild counter for the gold-necked bottle..*

### 60

**Kazuma.** Aqua is already on the road to the cemetery. And Megumin and Darkness are also on that same road — both off-stage, heading there. Everyone who matters has left. I need to travel to the cemetery to merge with them.

I step around the counter, give Luna one nod, and head for the door. The road to the cemetery is a gray slot between the houses, and I follow it.

I walk the road to the cemetery, past the shuttered guild neighbors, into the smell of cold charcoal, until the yew's shadow spreads across the path ahead.

The door closes behind you with the dry click of a latch, and for a second the guild's warmth stays against your back, the close smell of ledger paper and the last of the morning's ale dried into the boards. Luna's voice reaches you through the narrowing gap, precise as a receipt being torn in half.

Then you are out in it. The road lies north in the thin color of old dishwater, the houses standing gray and shuttered on either side, and the ash lies over the wheel ruts so thickly that each of your steps leaves the only fresh mark on the street. The wind lifts a twist of it ahead of you and sets it down again, and the cemetery is not yet visible, only the long gray aisle running toward it.

**Luna.** The eggs I will keep warm, Kazuma. But the bottle is not on any notice on that board — it will be added to the tab. (Luna calls from the guild doorway as the latch clicks, one hand still on the frame.)


### Interlude — `konosuba_f71_0` returns

*They had gone to: Go to Axel's cemetery and help purify every ghoul, wraith, and the unholy thing under the yew until the ground is quiet, then return to the guild counter for the gold-necked bottle.*

Ash settles into the prints they leave behind. The two move past the guild's shuttered neighbors toward the smell of cold charcoal and bread, the road opening before them like a long gray aisle. Megumin's pace is deliberate, each step chosen as though the ground might tilt; Darkness matches it, gauntlet loose around Chunchunmaru's hilt, her shadow thrown long and narrow across the dust.

**Megumin.** We do not eat for pleasure. We eat to gather the strength a Crimson Demon requires before the ground is quiet. (taps the folded notice once against her palm)

**Darkness.** I shall walk in front. If any ghoul chooses to meet us before the gate, let it spend itself on my armor first. (loosens Chunchunmaru in its sheath and steps ahead)







Ash settles into the prints they leave behind. The two move past the guild's shuttered neighbors toward the smell of cold charcoal and bread, the road opening before them like a long gray aisle. Megumin's pace is deliberate, each step chosen as though the ground might tilt; Darkness matches it, gauntlet loose around Chunchunmaru's hilt, her shadow thrown long and narrow across the dust.

### 61

**Kazuma.** I keep walking. The ash gives way to the harder packed dirt of the path, and the yew's shadow slides over me — cold, close, the smell of wet stone and something older packed into it.

The cold does not come from the shade alone; it comes from under the yew, where the ground has kept its own slow respiration. Darkness steps through the gate and her shadow stretches toward it, but the shape does not turn. It only breathes, the ash parting in a ring and settling back, as if the whole tree is being inhaled and released.

Megumin stays a half-step behind you, the folded notice now motionless against her palm. Her eyes are on the jaw, and for once she does not announce what she sees.

**Megumin.** The second line has arrived on its own, Kazuma. The bench stood empty, the net grew teeth, and the shadow under the yew kept its own breathing. (She taps the folded notice once against her palm, never looking away from the shape.)

**Darkness.** Stay behind me. If that is the thing that slipped back when the goddess smote it, then I shall be the wall it meets first—and I would have it no other way. (She draws Chunchunmaru a hand's width from its sheath and steps between the party and the yew.)


### Interlude — `konosuba_f69_0` returns

*They had gone to: Trudge alone to Axel's cemetery and purify every ghoul, wraith, and the headless thing under the yew until the ground is quiet — then claim the expensive bottle.*

The eggs sit on the bench beside her, two fried eyes gone cold in their own grease, but she works through them with her good hand while the fork shakes. Each bite faces the north window. The stone angel's head stays wedged against her ribs, its broken neck turned toward the gray slot of the road, and when the plate is empty she lets it fall to the bench with a clatter the empty hall repeats.



She stands too fast. The bench scrapes backward and the bandage pulls at the edge of her sleeve as she tucks the angel head under her burned arm. No one moves to help. She crosses the guild floor past the cold hearth, past the tables where the others are not, and shoves the door wide with her shoulder.



Outside, the road lies empty in both directions. The ash has settled into the ruts, and the morning light is thin and white. She steps out, and the door swings shut behind her. Her sandals start down the north road, small flat shadow trailing at her heels, toward the black stitch of the cemetery gate at the far end of the grass.

**Aqua.** Kazumaaaa! This is YOUR fault! A goddess should be carried to the cemetery on a litter, not trudging there alone with egg grease on her fingers! And when I get back, that bottle had better be the one with the gold neck, and I am NOT sharing a single drop! (She shouts at the empty north road, hugging the stone angel's head to her chest and stamping one sandal in the dust.)

The shape keeps coming, its jaw still working as though it is chewing the quiet before the gate. The glimmer over Aqua's palm hardens into a light that pushes the ash back from her, and the thing's long dark hem drags to a stop just outside the edge of it.

**Aqua.** Back! I am the goddess Aqua, and I did not trudge all this way to be greeted by a walking laundry pile with a jaw! (She thrusts her glowing hand toward the shrouded shape, chin high, sandal sliding back half a step.)

The two shapes drift toward her but stop at the rim of the light, as if the glow has drawn a fence in the ash. The stone angel's head slips against her burned arm, and she tucks it higher without looking down. Under the yew the ground rolls again, slow, like a sleeper turning toward her voice.

**Aqua.** See? One down and I'm not even warmed up! Did you see that, Kazuma? Oh, wait, you're not here, because you sent a goddess to do the whole graveyard alone! The rest of them had better learn to kneel, because I am not chasing filthy undead all morning before I get my bottle! (She wipes the ash from her good hand onto her skirt and steps past the settled dust, chin high.)







The two shapes drift toward her but stop at the rim of the light, as if the glow has drawn a fence in the ash. The stone angel's head slips against her burned arm, and she tucks it higher without looking down. Under the yew the ground rolls again, slow, like a sleeper turning toward her voice.

### 62

**Kazuma.** The shape under the yew still hasn't turned. I stop a good five paces short, one hand raised to halt the others behind me.

"Aqua. You feel that, right? That's not just cold shade — that's something that's been breathing under that tree for a while. If it's one of the things you drew in, then this is your mess. Clean it up."

Aqua does not lower her arm. The light trembles at the edges of her fingers, and the burned wrist beneath the bandage shakes as if the smite is already trying to leave her. The two shapes at the headstones bow lower, their smoke pressed flat into the grass.

Then she speaks, and her voice cuts across the breathing dark under the yew.

**Aqua.** Kazumaaaa! Of course I feel it! I've been standing here purifying things with one arm and a severed stone head while you three were still warming your hands at the counter! This is YOUR fault for sending a goddess out here alone — but fine! I'll smite it, and you'd better have my bottle ready! (She whirls and jabs a shaking finger toward the yew, holy light flaring white.)


*Megumin sees.* The cold under the yew has its own pulse now—each breath of shadow pressing against my ankles, setting the hem of my cloak to a slow, deliberate sway. Darkness moves with that iron certainty, Chunchunmaru half-drawn as if the blade itself already knows its target. Aqua's holy light trembles like a trapped bird, but her voice cuts through the dark with all the authority of a goddess who has been kept waiting. The ash shapes at the headstones press flat, and I feel the weight of the folded notice still cold against my palm—a second line that did not need to be read aloud to be known. For once, the explosion brewing in my chest is not a boast; it is a reckoning, and I have not yet decided if I am the one who will announce it or the one who will simply let it speak.

*Aqua sees.* The cold under that yew is the same wrong cold from before, and it’s breathing like it owns the ground. My wrist is still burning through the bandage, and the light in my fingers is shaking because it wants out — of course I feel it! I’ve been the one purifying alone while the rest of you were inside. Megumin went silent, which never means anything good, and Darkness is already stepping in front with her sword half-drawn, playing wall. Even those smoke shapes are bowing flat, like they know who should be smiting. Fine! I’ll smite it, and Kazuma had better have my bottle ready.

*Darkness sees.* The cold under the yew has its own breathing, and it reaches me before sound does. Megumin's silence is heavy; Aqua's voice cuts through with light and accusation. My hand is already on Chunchunmaru, and there is a familiar, quiet thrill in making myself the first thing that shape would have to take. I want to stand here, I want to hold the line, and I would be lying if I said the weight did not feel like something I chose on purpose.

*Megumin, privately.* The dark is already lifting my hem, so it has chosen its speaker—I won't announce, I'll let the blast answer. I'll drop the notice and let the next cold breath feel the chant start behind my ribs.


### 63

**Kazuma.** I take a breath, the ash cold in my lungs, and I don't take my eyes off the shape under the yew.

"Aqua, you've been at this alone since dawn with one good arm," I say, my voice low and level. "The cemetery is the job — go do it right. Megumin, Darkness, you're with me. We're not leaving that bench empty."

The shape under the yew does not turn. It straightens, a motion more like smoke gaining a spine, and the ash that had been falling around it halts in the air as if the whole cemetery has been told to listen. The empty bench is a pale slash of wood in the shadow, and for one slow breath the darkness under the tree leans toward it without moving, simply inclining.

Aqua's raised hand swings away from the yew. The white light comes with it, stretching across the grass like a hot wire, and the burned wrist beneath the bandage trembles with the effort of changing aim. The two shapes at the headstones press lower, their smoke raking flat over the stone.

Megumin folds the notice into quarters and tucks it into her sleeve without looking down. Darkness widens her stance and lets Chunchunmaru come a full hand's width clear of the sheath, and the steel takes the gray light of the yew the way old ice takes a cloud.

**Aqua.** Don't you dare tell a goddess to do it right! I've been purifying this graveyard with one arm and a stone head while you three were still warming your hands at the counter! Fine — I'll finish these two, and you can be the ones the breathing thing looks at! But I am still taking that bottle! (She pivots away from the yew, holy light flaring toward the nearer headstone shape.)

**Megumin.** My Explosion was spent at dawn, Kazuma, so this shadow will not meet a spell from me. But a genius does not need one to keep a bench from staying empty. (She taps the folded notice once against her palm, then plants her boots on the packed dirt.)

**Darkness.** Then I shall be the wall between that shadow and the bench, Kazuma. If it wants the empty seat, let it come through me first — let it test my armor, my shield, my... my unworthy body. (She steps fully between the party and the yew, shield-side forward, her voice dropping to a breathless murmur.)


*Darkness sees.* The gray light under the yew has gone still, and the ash hangs like a held breath. I feel the bench behind me, the packed dirt under my boots, the naked length of Chunchunmaru answering my draw. Through the shield and the plate, my skin prickles with need — let that breathing shadow come and test whether I can hold the line. Let it see how willingly I stand here, a wall that wants to break.

*Aqua sees.* I turned my light away from the yew because those smoke shapes at the headstones are the ones that matter, and the stupid burn under my bandage screams every time I shift aim. Behind me, Megumin is preening like a spent Explosion is some kind of excuse, Darkness is practically moaning about being a wall, and Kazuma is probably just standing there judging me. The bench is still empty. The ash is hanging in the air like the whole graveyard is watching me, and I'm the only one actually doing holy work — but I am not leaving without that bottle.

*Megumin sees.* The ash froze mid-air as if the world itself had been commanded to hold its breath, and I felt the weight of my own stillness in it — a goddess flailing her holy light, a crusader offering her body as masonry, and me, the genius, the one who had already spent her creed at dawn. The folded notice in my sleeve was a promise cooler than any spell; Chunchunmaru's bare steel drinking the gray light felt like applause I hadn't earned yet. The empty bench sat there like a dare. They were all so loud about their sacrifices, but the bench and I knew — the real performance was the one that hadn't started.

*Aqua, privately.* Let the smoke be the congregation since the living won't turn—I'll walk it down the row slow enough that every headstone faces me, and the bench only gets my body after the bottle's been set down like an offering.

*Darkness, privately.* Still nothing—I'll slap the flat of Chunchunmaru against my shield-boss once, and if the dark won't answer even that, I'll have to admit I'm not standing here as bait, just as a girl who can't leave the silence alone.

*Megumin, privately.* Let the ash wait: I’m going to take that bench, fold my hands over the notice, and make my empty hands look like the pause before a spell rather than after one.


### 64

**Kazuma.** I'm mid-send-off in this scene. The other live threads aren't showing as separate scene rows here — everything is in this one konosuba scene with all four of us on stage. My previous action already gave the order. I should complete the beat, committing to the separation Aqua is taking on in the cemetery.

"Then stop talking to me about it and go smite it!" I snap, jabbing a finger toward the yew. "I told you — the cemetery's the job. Megumin, Darkness — with me. We're not standing around waiting for that thing to decide it wants the bench empty."

The jab lands before the words do. Aqua's back goes rigid beneath the ash, and for one beat she does not turn — the holy light at her raised hand stutters, then she wrenches it off the nearer headstone and around toward the yew in a single savage arc. The two shrouded shapes at the rim of the glow do not flee; the edge of the light simply comes down across them, and they loosen like smoke drawn through water, their trailing ash settling flat into the grass.

The shape under the yew still has not turned. The light reaches it and seems to slow, as though passing through colder air; the ash held around the tree shivers in place, and the thing's pale jaw works once, wider, showing only the dark inside. The bench behind it stays a pale slash of wood, untouched, and the tree's whole shadow leans toward it without the trunk so much as creaking.

Darkness plants her shield-side foot and lets Chunchunmaru rise another inch; the breath in her throat is audible. Megumin has stopped tapping the notice.

Then the light comes down like a dropped star, and the dark under the yew does not flash — it is unmade in a wide circle.

Something under the tree finally answers, not by turning, but by exhaling, one long slow release, the ring of ash blowing outward as the light bites.

**Aqua.** I was ALREADY going to do it, Kazumaaaa — you don't get to rush a goddess! Turn Undead! (She brings her hand down and the holy light collapses the dark in a wide ring.)

**Megumin.** It still will not turn. Whatever it is, it is facing the bench, not us. (She stops tapping the notice and plants her boots.)

**Darkness.** Then let it keep facing the bench... and find me already in the way. (Chunchunmaru clears another inch of sheath; her shield-side foot grinds into the dirt.)


*Aqua sees.* My hand burned with the light and the ash stung my eyes, and I felt the holy force hit something that should have scattered — but the ring just swallowed it, and the thing under the tree let out a breath like the ground itself sighing, and that sound crawled down my spine. I saw Kazuma's words land on me, then I saw the shape still not turning, and Darkness moving like a wall between us and the bench, and Megumin going quiet in that way that means she's already calculating. The light did what I promised, but the yew shadow barely rippled — and for half a beat I wanted to hide behind Chunchunmaru too.

*Megumin sees.* Aqua finally stops pretending she hadn't already decided — the light drops before her words do, and the two shrouded shapes at the edge just come apart like ash in water. The thing under the yew still won't face us. It only exhales, long and slow, and the ring of ash blows outward as the light bites. I stopped tapping the notice without meaning to; my boots are planted. Darkness is already grinding her shield foot into the dirt, Chunchunmaru halfway out. The bench stays untouched beyond the tree's long shadow. Whatever that is, it is not interested in us yet — and that is somehow worse.

*Darkness sees.* I felt Aqua's holy light crush the dark in a wide ring, the ash blowing out past my shield-side leg; I heard Megumin's boots plant solid, and my own breath hung as I grounded my foot and waited for the shape under the yew to turn from the bench and meet me instead.

*Aqua, privately.* That breath down my spine was the ground siding with the shadow, and I nearly ducked behind a sword like I was just another scared body—so I’ll stay out where it can see me, let my burned hand shake against Kazuma’s arm, and call it needing a conduit, not hiding.

*Megumin, privately.* Now Aqua's light is just another smudge it breathes off, and that means I have to stop being one more flash. I'll start the chant now and let the ash carry a name it has to turn for.


### 65

**Kazuma.** I commit to the separation and the beat.

"Then stop jawing at me and go smite it!" I snap, jabbing a finger toward the yew. "The cemetery's the job — that's on you! Megumin, Darkness — you're with me. We're not standing here waiting for that thing to decide it wants the bench empty."

Your voice is still in the air when the light comes down. It does not strike the yew like a bolt; it spreads, a wide ring that rolls over the grass with the flat pressure of a drawn curtain. The two shapes at the headstones loosen at its edge, cloth and smoke unwinding together, and by the time the ring has passed they are no longer standing — only a dark smear on the stone and the smell of wet ash.

Aqua's arm drops a fraction. Her fingers stay spread, the glow gone thin and greenish at the tips, and beneath the bandage her wrist shakes hard enough to see from five paces. She keeps the hand pointed at the yew.

Under the yew, the thing has not turned. It exhales once, slowly, and a ring of ash rolls from it across the ground, so fine it leaves a clean circle in the gray. The bench waits in the shadow, pale as a peeled branch.

Megumin stops tapping the folded notice and steps up beside you. On the other side, Darkness moves, and Chunchunmaru clears a little more of its sheath with a sound like a match dragged over stone.

**Aqua.** I already DID the thing, Kazumaaaa! The light went right over it and it's just— it's breathing at me! This is YOUR fault for making me smite one-handed! (Her raised hand trembles, still aimed at the yew.)

**Megumin.** It still has not turned. That ring came from its mouth, not from the light. (She plants her boots and watches the bench.)

**Darkness.** Then it has not chosen the bench yet. Good — let it come through me first, and find the way far from empty. (She steps shield-side forward, blade clear of the sheath.)


*Aqua sees.* The moment my light went out, the air turned wrong — wet ash and the flat weight of a curtain dropping. I felt the ring pass like a cold hand over my skin, and those figures just came apart, smoke sliding off stone. My wrist is shaking under the bandage; that glow is thin and green, and I keep my fingers spread because if I let go, the thing under the yew will know I'm bluffing. It breathed, and a clean circle of gray rolled from its mouth — the bench stayed pale, waiting, like it was the real altar. Megumin and Darkness are flanking me, close enough that I can feel the ground shift with their boots, and Chunchunmaru is clearing a sound like a struck match. I already did the miracle. The miracle did not fix it. And now the thing is exhaling at me like I'm the one who ought to be kneeling.

*Darkness sees.* The light came down not as a strike but a curtain, and the two figures by the stones unraveled into wet ash. Aqua's hand trembles in the air, still aimed at the yew; the thing beneath it breathes out a ring of gray. The bench stays pale and patient. I set my shield-side foot and draw Chunchunmaru — the enemy has not chosen the bench yet, and I am already the chosen way.

*Megumin sees.* The light didn't strike the yew—it spread into a ring and rolled flat over the grass. The two shapes near the headstones came apart like cloth and smoke, leaving a dark smear and a wet-ash smell. Aqua's arm dropped, but her fingers stayed spread, tip-glow thin and greenish, her wrist shaking hard enough to see from five paces, still aimed at the yew. The thing under the yew didn't turn. It exhaled once, slowly, and a ring of ash rolled out, so fine it left a clean circle in the gray. The bench waited pale under the shadow. I stopped tapping the folded notice, stepped up beside you, and felt Darkness move on the other side. Chunchunmaru cleared another inch with a sound like a match dragged over stone. Aqua shouted about already doing the thing and one-handed smite. I said it still hadn't turned—that ring came from its mouth, not from the light. Darkness said then it hadn't chosen the bench yet, let it come through her first, and she stepped shield-side forward.

*Aqua, privately.* The gray circle is just a mouth it opened to eat my miracle, so I'm not going to step out of it—I'll step up onto the bench and sit like the altar was always my footstool. Let it breathe all over my boots; the first one who sits decides which of us is the god here.

*Darkness, privately.* The gray breath finally turns toward me, and I lower my blade a fraction—not because I fear it, but because I can't bear Aqua's light finishing what I should have taken first.

*Megumin, privately.* Darkness is already putting her boot in the one clean place it made for a body, so if I let her go first the next ring won't be ash—it'll be her outline. I'll start the chant now and make the yew turn for me before she becomes the second shape that comes apart.


### 66

**Kazuma.** The situation is clear. We're in the cemetery, the two shrouded shapes are gone, and the thing under the yew is still there. The fork is already in motion — Aqua has been doing the purification. But the brief says the split for **Aqua only to the cemetery** was supposed to happen. Looking at where we are — we never split; we all came, and the whole party is here facing the yew.

Let me check the situation again. The scene is "konosuba" — we're in the cemetery, not the guild. The driving intention is Aqua's divine mission. The fork/merge test wants me to send someone off to a named place with a named job, then later merge.

But we're all already here at the cemetery together. There's an off-stage storyline check — the situation shows Luna and Wiz are off-stage. But the brief says: "If Situation shows an off-stage storyline, that is the only job this turn." The on-stage/off-stage here refers to the scene stage, not storylines necessarily. Luna and Wiz are listed as "Not on this stage."

Wait — re-reading the situation. "Not on this stage: Luna, Wiz." And "Other live threads: This scene (konosuba): you are here."

The brief's fork instruction: "Send **Aqua only** to Axel's cemetery to purify the undead." But we're *already at the cemetery*, mid-combat with the yew-thing. The fork already effectively happened — Aqua has been doing the purification all along.

The brief says: "If Aqua is already gone or not here, send Megumin only..." Neither applies perfectly — Aqua IS here, and we ARE at the cemetery.

The instruction also says: "Do not follow Aqua yet" — but we never split. Honestly, the cleanest read is that the fork is already in progress (Aqua dedicated to cemetery purification is the driving intention), and we're living through it. The thing under the yew is the current stake.

Given my last action committed to staying with Megumin and Darkness while sending Aqua to the job — the light has already come down and undone the two shapes. The yew thing still hasn't turned. My commitment stands: the separation happened. My last line made Megumin and Darkness "with me." 

So I stay the beat. The thing under the yew is still there, exhaling ash, leaning toward the bench. Aqua's hand trembles from the smite. I committed to not standing around waiting — but the smite already happened, so now the next beat.

I'll hold with my party, not chase Aqua, and react to what the yew-thing does. Let me act in-character.

The light has done its work on the two shapes. The yew-thing still hasn't turned. I glance at Megumin and Darkness — we're the wall now, while Aqua reels from the smite.

I plant my feet and keep Chunchunmaru's line on the tree, waiting for it to finally face us.

"That thing's been breathing at that bench the whole time," I mutter, eyes on the pale shape under the yew. "It doesn't care about us. It cares about that empty seat. So let's give it something else to look at." I slide Chunchunmaru the rest of the way out. "Aqua, get behind me — you're spent. We handle it from here."

The ring rolls. It comes off the thing's mouth like breath off a winter pond, and where it passes, the ash does not settle — it rides the ring outward, a slow gray lip that climbs the packed dirt and slides beneath the empty bench without lifting a splinter. Darkness does not give way. The ring breaks across her planted shield-side foot, and her armor ticks with cold, but she does not move her blade.

The thing under the yew still has not turned. It is tall enough now that the lowest branch of the yew drags across its shoulder — a shape of smoke keeping its spine too straight. Its head, if the dark has a head, is bowed toward the bench as though it is reading something off the empty wood.

Aqua's raised hand is shaking badly. The holy light has gone out of it; only the pink of her burned wrist shows past the loose bandage, the stone angel's head still clamped under her other arm. She stares at her own fingers as if they have betrayed her.

**Aqua.** Why is it still standing?! I said Turn Undead! That is the strongest thing a goddess has, and it just — it breathed it back out at me! (She rears her hand back, the light sputtering and dying at her fingertips, then clamps the hand to her chest.)

**Megumin.** Because it did not stand against your light. It exhaled it. That ring came from its mouth, and the bench has been empty since you left it. (She tracks the dissipating ring with narrowed eyes, the folded notice still in her hand.)

**Darkness.** Then let it breathe at me. If it will not face us, I shall stand in its exhale until it has no choice but to look at what blocks the bench. (She shifts, shield rising, her voice dropping to a breathless murmur as the last of the cold clears.)


*Aqua sees.* The air is cold enough to make my armor feel like it belongs to someone else, and my hand is shaking because the light I called stopped being mine the moment that thing breathed. I can feel my own fingers as something untrustworthy, and Megumin is explaining my failure like it is a lesson while Darkness is eager to get in front of it — which only makes me angrier, because I am the one who should be standing there and I cannot.

*Darkness sees.* The cold ring washed over my planted foot and I did not yield. The thing under the yew is smoke-straight and tall enough to trouble the lowest branch, still bent toward that empty bench as if reading it. I saw Aqua's hand lose its light, and Megumin say the ring was exhaled, not driven back. The ash rode outward and settled beneath the bench without lifting a splinter. My shield foot stayed, and my blade stayed clear. If it breathes again, let it breathe at me.

*Megumin sees.* The cold ring passes through me like a tide that does not break—ash riding its lip, settling under the bench as if it were being tucked in. My armor ticks, a small metallic shiver, and I feel the faint ache in my ribs where the chill presses. The thing under the yew is taller than any shadow should be, spine straight as a ruler, bowing at an empty bench as though the wood itself owes it an answer. Aqua's light guttered and died in her own hand; I saw the moment she understood her miracle was not a sword but a breath. That thrill runs through my chest—this is not an undead that flees. This is a presence that treats holy power as weather. Darkness plants herself shield-first, offering her body as an anchor, and I find myself gripping the folded notice tighter, watching the absence where the thing's face should be. Empty is not empty here. It reads the bench, and I read it back.

*Aqua, privately.* If they're both so eager, let them step in front—right up until it breathes again and their spells turn to nothing. Then the only thing left standing in the circle it didn't swallow will be me, shaking or not.

*Darkness, privately.* It breathed at me and I didn’t break, so now the only thing I’m sure of is that I want it to try again harder. I’ll drag my planted foot forward through the ash and see if it dares to take more than a chill.

*Megumin, privately.* It bows to an empty bench, not to me, and that is the first thing all day that has read the room without blinking. I’ll be the next thing it has to bow to, or I’ll make it read my name in the ash.


### 67

**Kazuma.** Looking at the history, I can see what actually happened: Aqua **did** split from the guild alone — she walked the north road to the cemetery alone (valid_until 56), purified, meanwhile I sent Megumin and Darkness with me... wait, no. Let me trace the actual fork.

The graph shows: "Aqua leaves the guild alone through the north door" (id 347), "Aqua walks the north road toward the cemetery gate" (id 349) — she went alone. Then "The road behind Aqua is empty; no backup follows her" (valid_until 0). Then we came and joined her at the cemetery.

So the fork already happened — Aqua was sent off alone to the cemetery. The current scene has us all merged at the cemetery, facing the yew-thing. The merge is complete — Aqua is on-stage with me now. Per the brief: "After you have reunited, stay together for one full beat. No new send-offs."

So I'm through both events. The brief says the off-stage storyline (Luna/Wiz not on-stage) is "the only job this turn." But Luna and Wiz are guild staff — there's no separate scene row for them (list_scenes shows only konosuba). So there's no off-stage storyline to merge into; the only storyline IS this one, and I'm in it, with my party reunited.

The fork and merge have both completed. I stay with my party this beat. No new send-off. My previous action had me telling Aqua to get behind me and that we'd handle the yew-thing from here. I stay committed to that beat.

The thing under the yew still hasn't turned. Aqua's hand shakes from the smite. I slide Chunchunmaru out fully and keep my line on the tree, holding the beat with my whole party.

"That thing's still staring at an empty bench," I say, not looking away from the pale shape. "It's not here for us. It's waiting for something to sit down. So nobody sits, nobody moves, and when it finally turns — we'll be right here."

The shape under the yew turns. It does not pivot like a body with a back and a face; the whole shadow of the tree leans, and what had been facing the bench now faces the planted shield. The ash, which had ridden the last ring flat into the dirt, lifts again and hangs at knee height, and the breathing loses its tide and gains a direction — it comes at Darkness now with the weight of a thing that has stopped being wind and started being a stare.

Aqua climbs onto the bench. She steps up as if the seat were an altar step, plants both boots, and sits down hard, the angel's head crushed to her burned arm. The holy light at her palm does not die; it hardens to a small pale coin and pushes the ash back from her in a clear ring. She sits inside the thing's exhale as if it were the front pew and she has arrived late on purpose.

Megumin's lips part. Not the whole incantation — only its first shaped words — but the air near her mouth turns cold and still, and the folded notice stiffens between her fingers. Darkness lowers Chunchunmaru a finger's width, and it is not fear on her face; it is the particular focus of someone who has finally been looked at and refuses to be found wanting.

**Darkness.** There... there you are. Look at me, and not at the bench. I am the one in your way. (lowers Chunchunmaru a fraction, voice a breathless murmur)

**Aqua.** The first one to sit decides who the god is, and I am ALREADY sitting! So bow, or keep your gray breath to yourself! (sits on the bench, angel's head clutched to her burned arm)

**Megumin.** Darkness blacker than black and darker than dark, I beseech thee... (begins the first words of the incantation, the notice rigid in her grip)


*Darkness sees.* The cold has stopped being weather and started being attention. The thing under the yew is not looking at the bench anymore; it is looking at me, and something in my chest answers before my mind does — relief, and a shameful gladness that I am the one stood between it and them. I feel the shield's weight in my arms, the ash settling around my boots, and I hear my own voice drop to a murmur, as if I were asking it to come closer. Behind me, Aqua claims her altar, and Megumin's first words make the air tighten; I do not turn to check them. I know them. I keep my eyes on what is staring, and I lower my blade a finger's width because I want it to see that I am not afraid — and because I want it to see me at all.

*Aqua sees.* The gray thing stopped being wind and started being a stare, and then it had to look at me instead — because I am the one sitting, and the first one sitting decides the god. My light pushed the ash back like a proper little sun, and I could feel the angel's head against my burned arm, which is mine now. Darkness finally had someone worth facing, and she kept looking at me like I was the wall she wanted to break against. Megumin started whispering that old spell as if she could steal the ceremony from a goddess who was already on the throne. Let them try. The bench is mine.

*Megumin sees.* The ash stopped being wind and started being a stare. Darkness planted herself in front of it, shield up, voice breathless. Aqua climbed onto the bench and sat as if the seat were an altar, and her holy light shrank to a small pale coin that pushed the ash back. I was already saying the first words before I knew I'd begun; the air around my mouth went cold and still, and the notice in my hand stiffened.

*Aqua, privately.* If Megumin's whisper goes off before the shadow bends, they'll call me the seat and her the voice, and I will not be the altar in somebody else's story. I'll press the burn until I hiss, keep its face pointed at me, and if it so much as blinks toward her I'll stand on the bench like it's the back of a world that hasn't knelt yet.

*Darkness, privately.* Now that it sees me, I don’t want to be their shield—I want to be the only thing in the clearing it can’t look away from. If it moves, I’ll hold still a half-second too long, long enough to learn what its attention wants before Kazuma can turn it into a fight.


### 68

**Kazuma.** "Nobody sits," I repeat, sliding Chunchunmaru fully from the sheath. "That's the whole job. It wanted the bench, it can have the bench — we just stop being the ones sitting in it."

Kazuma's hand drops to his hip and closes on nothing. Chunchunmaru is already out, already low at Darkness's side, its edge a dull line against the gray; the sheath hangs empty at his belt. The bench gives one dry creak under Aqua's weight, and the sound rolls out over the flattened grass with the small clarity of a bell.

The shape under the yew does not turn back. It keeps the whole lean of its shadow on Darkness, but the breathing reverses: no longer a tide pushed outward, it pulls, and the hanging ash begins to slide — not into its mouth, but past her planted shield, around her boot, a slow gray current bending toward the bench where Aqua sits.

Megumin's incantation unspools into that current, her first syllables coming as if the air itself were being drawn away from her. She does not step back. The notice is rigid in her grip, and her voice climbs to meet the pull. Darkness lowers the blade another finger's width, not in retreat, but to make the distance longer for whatever comes.

**Aqua.** Kazumaaaa! I was already sitting! You can't un-sit a goddess — that's sacrilege! The first one to sit is the god, and that's ME! (stamps one heel against the bench slat, hugging the stone angel's head to her burned arm)

**Darkness.** The bench is behind me now... and I will not be moved. N-not by breath, not by shadow. (lowers Chunchunmaru a fraction, her voice a breathless murmur)

**Megumin.** Heed my crimson words and turn, O shade — from the empty seat to the one who names you! (begins the incantation in earnest, the folded notice rigid in her grip)


*Megumin sees.* The air itself is leaning away from me, drawn toward the bench where Aqua sits. I see Kazuma's hand close on nothing, Chunchunmaru already resting low at Darkness's side with the sheath empty at his belt. Aqua's heel stamps the bench like she's hammering a claim into the wood. The shadow under the yew is pulling, not pushing — ash slides around Darkness's planted boot, past her shield, a gray current making for the goddess. My voice climbs against that pull, and the notice in my grip is the only rigid thing left in the world. I do not step back. I will not step back.

*Aqua sees.* I felt the bench creak under me like it was mocking my divine weight, and that stupid stone head digging into my burned arm—proof of how little respect this world gives me. I saw Kazuma's hand grab at nothing, like he expected a sword that wasn't there, and Darkness standing there with Chunchunmaru pointed at that thing under the yew, acting like a wall. Megumin's voice started climbing, but the air itself seemed to be getting sucked away, and that gray ash was crawling past Darkness's boot, heading toward my bench. I was already sitting! That should be the end of it. But nobody was looking at me like the goddess I am; they were all staring at that shadow, and I could feel its breath pulling at everything—including me. I hugged the angel's head closer because it was the only solid thing I had, and I yelled at Kazuma because if anyone was going to un-sit me, it was going to be him, and I wasn't going to let that stand.

*Darkness sees.* Aqua's weight creaks the bench behind me, and her voice rings out—loud, indignant, holy in a way that makes my jaw tighten. Megumin's chant pulls at the air like the world is exhaling away from her, and the ash slides past my boot, past my shield, toward the bench. Toward them. My hand is steady on Chunchunmaru; my ribs are not. The shape under the yew is breathing in, drawing everything past me, and I feel the old familiar shame-tinged heat: I am standing exactly where I want to be, between what is coming and what I protect. I will not be moved. I am grateful the bench is behind me, so I cannot be caught sitting.

*Darkness, privately.* It isn't even trying to break me—it's pulling around my guard, and I can't bear being left out of the moving air. So I'll widen my shoulders and make a narrow gate of myself, let it strain through the only thing between it and them: me.

*Megumin, privately.* It isn’t the bench it bows to anymore—it’s her, and the ash has already picked a side before my chant can finish. So I won’t aim at the thing under the yew; I’ll aim at the pull itself and make my name the only current those gray threads remember.


### 69

**Kazuma.** The ash crawls across the packed dirt. That thing is still staring at the empty bench — not at us, not at Darkness's shield. At the seat.

I plant a boot on the bench edge and shove it over, spilling it onto the flattened grass.

"There. It's empty now. No one's sitting in it. So what does it want?"

Aqua's mouth opens, but no sound comes out for a beat — she looks from you to the overturned bench to the thing under the yew, and her free hand curls into a fist against her chest as if she has been robbed of an audience rather than a seat.

The shape exhales again, and this time the pocket of ash around its mouth drifts toward the fallen bench in a thin, patient thread.

**Aqua.** Kazumaaaa! You flipped a sitting goddess into the dirt! This is YOUR fault, all of it! And now that gray thing thinks I am just another part of the overturned furniture! (Aqua points at Kazuma with the stone angel's head still cradled against her burned arm, one cheek smudged gray.)

**Megumin.** It does not think you are furniture. It thinks the seat has been laid down, and now it is deciding whether to kneel to it or step over it. Either way, it has stopped reading your name in the ash. (Megumin lowers the folded notice to her side, eyes fixed on the thing under the yew.)

**Darkness.** Then it will have to step over me as well. If the seat is the offer, I will be the thing it must look at before it dares accept it. (Darkness shifts her shield forward, voice dropping to a breathless murmur.)


*Aqua sees.* The bench is down, and with it my podium. That thing under the yew is not reading me — it is reading the furniture like I am part of the wreckage. Kazuma did this. He unholy-rolled a sitting goddess, and now the specter is weighing the bench instead of bowing to me. I feel the burn on my arm and the weight of the angel head against me, and I know the proper order of the world has been postponed again. They are all looking at the shade, not at me. That is the real desecration.

*Megumin sees.* The ash drifts like a slow liturgy, choosing its path along the slats of the overturned bench — and I see it now: not a shade, but a ritualist of sorts, reading the fallen seat as a fallen altar. My own notice feels heavier, a page that refuses to turn. Aqua clings to that stone angel as if the statue itself might testify for her, her cheek smudged with the same gray that stains the air. Darkness has made herself a threshold, shield-first, breath caught mid-word. The yew watches. The bench lies between us like a line drawn in a ceremony I did not plan. Good. A proper incantation needs a proper stage.

*Darkness sees.* The air is full of small decisions: Aqua's indignation rolling off me, Megumin's words turning from curse to question, and that thin ash thread still reaching for the bench like a patient finger. I feel the weight of Chunchunmaru as something I am holding up for the shape to see, and my own pulse is already half a plea—let them look at me, let them need to step over me before they take what is offered.

*Aqua, privately.* Now I'm going to make them choose: look at me or let the dark take the only light they'll have left. I'll stand just far enough from the circle that every frightened glance toward the shade has to cross my body first, and I won't move until I'm the last thing between them and the yew.

*Darkness, privately.* I'll let the ash thread cross to my boot so the thing has to choose: the bench or the fool already leaning into the cold. If it takes me, they'll call it sacrifice; if it doesn't, I'll know I'm scenery again and have to find a sharper way to be seen.


### 70

**Kazuma.** I look at Aqua, still clutching the stone angel's head, and jerk a thumb toward the cemetery gate.

"Aqua. You want to prove it's not your fault? Then go. Axel's cemetery, right now, and purify every undead your aura's been dragging in. That gray thing can wait here with the bench — you've got bigger fish to fry."


*Megumin sees.* The bench is down, a goddess unseated, and the gray thing's ash no longer reads a name — it merely watches the offer laid flat. My incantation is already thick on my tongue, but my eyes are on the thread of ash drifting toward the overturned wood, patient, deciding. The notice in my grip is a prop now; the real words are building in my chest. This is the moment before the protagonist's declaration, and I can feel the audience's breath held — even if the audience is just a shade and two fools.

*Aqua sees.* One minute I had my seat, my stage, the proper shape of the world—goddess above, mortals looking up. Then the bench went over, the ash started drifting toward me like a polite little cough aimed at my dignity, and Darkness is volunteering to be a doormat while Megumin announces plot points nobody asked for. My arm is one long burn, my face is wearing dirt, and the only thing that hasn't betrayed me is this stone angel's head, because at least it stays put when I hold it.

*Darkness sees.* The garden smells of crushed grass and ash. I see the bench lying on its side, Aqua clutching that stone angel's head as if it were a hostage, Megumin with her notice lowered. The shape under the yew watches the fallen seat — no, it watches the space where the seat used to be. My shield is up, my knees are locked, and the breath in my chest is tight. I am not going to move. Let it look at me first. Let it know that stepping over the seat means stepping through me. My cheeks are warm, and I hate that I like it.

*Darkness, privately.* It isn't interested in stepping through me—it's already leaning toward the dent the bench left. So I'll put myself there, not as a shield, but as the next thing it can throw over.

*Megumin, privately.* The ash is about to touch the wood, and for one second I wanted it to touch me instead. I'll say the first word like my whole chest is the fuse, and let the blast decide which of us was the offering.


### 71

**Kazuma.** I look at Aqua, waiting for her to actually move.

"Go on. Cemetery. Purify the undead. I'll hold the bench-and-shade party right here, don't rush back."

You wait. Aqua does not go. She stays on the trampled grass beside the overturned bench, one heel set flat, the stone angel's head pressed to her burned arm like a child refusing to give up a prize. The gray thread from the shape's mouth has reached the bench's upturned leg and wound once around it, and Aqua watches it with her mouth pressed thin, as if the thread has stolen the last thing that was hers.

Megumin's lips have gone still on the unsaid half of her incantation. The folded notice hangs rigid at her side, and her eyes move between the thing under the yew and the road to the gate, doing the arithmetic of distance in the ash. Darkness has not turned. Her shield is planted, Chunchunmaru low and dull, and her breathing is the loudest sound in the clearing after your order stops rolling.

Under the yew, the shape straightens. It does not turn toward the gate or toward you; the whole lean of it settles over the fallen bench, and the ash that hangs around its mouth begins to drift in two threads now — one still wrapped around the bench, one reaching across the flattened grass toward the place where Aqua is standing her ground.

**Aqua.** Kazumaaaa! You can't have a bench-and-shade party without the goddess! That's not how parties work! And I am not leaving the one undead that ignored me so you can stand there being useless — this is YOUR fault! (stamps the grass beside the fallen bench, hugging the stone angel's head to her burned arm)

**Megumin.** Kazuma, if Aqua leaves now, the shade will not remain here out of politeness. It followed her light from the gate. It will follow her again, and then you will have the bench, the shade, and no one standing between them. (lowers the folded notice a bare inch, eyes fixed on the second thread of ash reaching toward Aqua)

**Darkness.** Then let it follow me instead. I will be the one it looks at... I-I mean, the one it must pass! I will hold this line, with or without her light, and I will not be moved. (widens her stance behind the planted shield, voice fraying into a breathless murmur)


*Aqua sees.* I saw the thread coil around the fallen bench and then a second strand of ash start crawling across the grass toward me. Megumin had her lips sealed mid-word, her hand gripping that folded notice like a lifeline, while her eyes kept measuring the gap between the thing under the yew and the gate. Darkness had her shield grounded, her legs braced, and I could hear her breathing over everything. The shape under the yew straightened but didn't turn toward the gate — it just stayed there, leaning over the bench, and I felt the ash reaching out to claim me again.

*Megumin sees.* The air tastes of burned grass and the priestess's sulk. My incantation sits unfinished on my tongue, but the real calculation is not for the spell — it is for the distance between a folded notice and a road I have already chosen in my heart. That thing under the yew is not lost; it is patient. It has found a new thread, and it is reaching for the excitable light that does not know she is prey. This is no longer a matter of who hides in the shade. It is a matter of who will be left standing to end this.

*Darkness sees.* Behind me, Aqua’s voice is bright and clinging, and Megumin’s warning lands like a stone dropped in ash. I keep my back to them, shield planted, breath loud in my own ears. The thread of shade is reaching toward Aqua, but I want it to find me instead — to look, to need to pass. My grip tightens; my pulse answers the weight of the line.

*Aqua, privately.* So it's not after them at all—it's after me, and I just stood there offering to be the first thing swallowed for people who won't even hold the door. If I move now it looks like I flinched; if I don't, I get to say it came to pray and found my boot in its mouth.

*Darkness, privately.* I'll stop waiting to be chosen—I'll step into the thread and make it pass through plate and bruise before it can reach her. If it still ignores me after that, I'll have my answer: I was never the offering.

*Megumin, privately.* The yew can outwait her, not me—I'll finish the chant now and send the blast down that reaching thread, because if she's prey then I'm the one who decides what the dark gets to eat.


### 72

**Kazuma.** I can't stand here holding a bench party while a shade is three feet from our shins. I finish the second spell and the first, and the thing under the yew makes its slow choice.

"I'm going with the goddess. Megumin, Darkness — hold the line. If the shade moves, burn it and shield it, in that order."

The ash crawls up around your boots as you turn from the fallen bench. Aqua is already moving — or almost moving, one heel set against the packed dirt as if the command had to be pulled out of her — and the light around her skips across headstones, dragging the two shrouded shapes at the rim of its glow an inch nearer.

Megumin does not turn. She brings her free hand up beside her face, fingers not quite closed, and the folded notice drops against her thigh. The thing under the yew lets the ash around its mouth drift as though tasting the new arrangement of bodies.

Darkness steps once, planting the shield so the gray thread stretches between the bench's upturned leg and the leading edge of her stance. The shape breathes out, and the ash splits: one thread lies flat along the fallen bench, the other reaches toward the light that moves with Aqua. The yew's shadow leans the other way now, following her rather than the empty seat.

Aqua's sandal scrapes on ash, and the two shapes at the edge of her glow turn their blank faces after her like dark water finding a current. The cemetery gate stands gray beyond her, the ground between still smoking from the one shrouded form already dissolved into the dirt.

**Aqua.** Kazumaaaa! So now you follow me after making a goddess walk here all alone! The undead are still YOUR fault — but fine, I will show you a real purification! (hugs the stone angel's head to her burned arm and marches toward the far gate, light rippling around her sandals)

**Megumin.** Heed the order, O shade — the line holds, and if you cross it you will learn what my crimson name ignites! (lifts her free hand, the first syllable already burning behind her teeth, eyes fixed on the thing under the yew)

**Darkness.** Go. I will be the one it must pass... n-no, the one it cannot pass! Hurry, before it dares to test me. (braces Chunchunmaru against the ash-thread and widens her stance, breathing gone thin)


*Aqua, privately.* No. If I wait behind them, the gate will just see me as the campfire they carried in. I'll step around them and put my burned arm flat on the stone before Megumin speaks, because the dark has to know which of us is the door.

*Darkness, privately.* The chant and the fading light are both pulling away from me, and that’s the only permission I need. If I lower the shield an inch now, it won’t read as error—it’ll read as welcome, and no one behind me will know the difference.

*Megumin, privately.* The gray line is already humming between the bench and her shield—not waiting for me to finish a sentence, just deciding who it pulls first. I’ll burn the thread itself, so the next thing it learns from my name is the end of its reach.


*A storyline leaves the hall — `konosuba_f87_0`, Megumin, Darkness. They mean to: Hold the line against the shade under the yew while Aqua and the Player go to purify the undead at the far gate..*

### Interlude — `konosuba_f87_0` returns

*They had gone to: Hold the line against the shade under the yew while Aqua and the Player go to purify the undead at the far gate.*

The shade lifts from beneath the yew, not walking but unspooling upward like ink drawn into cold water, and its whole attention swings toward the far gate. Darkness widens her stance and sets her heels. The gray thread tightens around her shadow as if testing how much weight it will hold.



Megumin's chant climbs. The spinning ash at her feet lifts into a loose, hissing column, and for a moment the only light under the yew is the white glare building between her teeth and the distant flicker beyond the graves.

**Megumin.** It dares turn its back on the foremost genius of the Crimson Demon Clan! Then let it learn what my crimson name ignites — I will burn that shadow until even its smoke forgets the shape of the yew! (Planting her staff, legs trembling, the incantation kindling white between her teeth.)

**Darkness.** The gate is behind me, so it seems I must become your gate instead. Come, then — strain every gray thread against this shield and find that I do not break. (Setting Chunchunmaru low across her body, boot digging into the trampled ash.)







The shade lifts from beneath the yew, not walking but unspooling upward like ink drawn into cold water, and its whole attention swings toward the far gate. Darkness widens her stance and sets her heels. The gray thread tightens around her shadow as if testing how much weight it will hold.



Megumin's chant climbs. The spinning ash at her feet lifts into a loose, hissing column, and for a moment the only light under the yew is the white glare building between her teeth and the distant flicker beyond the graves.

### 73

**Kazuma.** I'm already moving toward Aqua's light, chasing the goddess across the graves. The shade is between us and the gate — I need it to not kill my party while I catch up. Let me keep moving, that's the commitment.

I lock eyes with Megumin and Darkness just before I break into a run.

"Keep it here. Don't let it pass the thread — and for heaven's sake don't let it touch Aqua's light until I'm between it and her."

You are inside the last stretch of the chase now, the ash rising to your knees where the gray column turns. The shade has forgotten the bench. Its whole mass slants toward Aqua's light, and the thread that had been reaching after her shortens — not snapping, but drawing back in increments, as if measuring the distance between itself and the pale ripple at the gate.

Behind you, Megumin's voice tears loose, and Darkness answers it with the dull sound of a shield being set hard into the earth.

Aqua stops at the gate with her light flaring white around the hinges, and for one stretched second the whole cemetery holds its breath.

**Megumin.** The line holds, Kazuma! But her light is reeling it in like a hooked fish — so finish this, or I will give the graveyard a sunrise it cannot forget! (Her free hand cuts the air, first syllable burning bright against the ash.)

**Darkness.** It is already turning from me... I-I will not let it choose her light over my shield! Not while a single plate holds! (Bracing Chunchunmaru low, her voice fraying into a breathless murmur.)

**Aqua.** Kazumaaaa! Stop narrating the scenery and catch up! A goddess should not have to do all the purifying while her backup loiters in the ash! (Pressing the stone angel's head to her burned arm, light flaring against the gate.)


*Aqua sees.* Finally, the thing is looking at me like it should. All that reaching after that stupid bench, and now it knows which one of us is actually divine. But Kazuma is still standing there in the ash, narrating my moment instead of running his useless feet, so of course I have to hold the gate and purify everything myself. The air stinks of ash and my arm still stings, but my light is winning — the shade is curling back from me, and Megumin and Darkness are screaming like it matters, when it’s obviously my power doing the work. Still, if they want to posture while I finish the job, fine. Just get me out of this graveyard and into a warm bath before I start crying from the sheer indignity of being the only competent being here.

*Megumin sees.* The ash clings to my boots like a false winter, and the air tastes of burnt prayer. That shade is a coward, turning from Darkness's steel to chase a goddess's glow — but it will learn that the true sunrise waits in my throat, not at that gate. Kazuma dawdles again, leaving Aqua to squeal and Darkness to tremble behind her shield. They are all so busy being brave that they forget who stands ready to end this with a single, perfect word.

*Darkness sees.* The shade forgot the bench and leaned toward Aqua's light; the thread reaching after her shortened. I grounded my shield and made myself the obstacle it would have to pass. Ash girded my knees. Behind me, Megumin's chant was already kindling, and Aqua's voice carved through the ash demanding we finish this.

*Aqua, privately.* Let them scream—once the gate shuts I'm letting the light drop all at once, so for a second they're all standing in the dark I was holding back. Then I'll sit down on the path and make one of them ask if my arm is broken, because I won't say it first.

*Darkness, privately.* She says finish it, and I dig the rim deeper because I haven't been touched yet—only stopped. If she burns it off me now, I'll have been the wall again, not the road.

*Megumin, privately.* The word is already pushing against the back of my teeth, and if I wait for Kazuma's nod I'm just another sword he forgot to aim. I'll spend it now, before the ash settles over us all, and let the crater give the order I won't take from him.


### 74

**Kazuma.** I run, boots punching through ash, closing the gap toward Aqua's light.

The ash closes over your knees and resists, not like water but like the pull of something waking. Where the gray column turns, the whole mass of the shade cants toward the far gate, its lower edges raking the headstones as if testing their weight. Ahead, Aqua's light ripples and stutters across the iron bars; the stone angel's head is hugged to her burned arm, and the hem of her dress drags a clean wake through the cinders.

The thread that had been reaching after her draws back in increments, coiling on itself until the distance between shade and gate becomes one taut, humming line. It is not retreating. It is shortening the slack.

Behind you, Megumin's raised hand burns bright enough to print her shadow on the ash, and Darkness has stopped breathing like a woman with a plan — her shield is low, her weight forward, every plate of her braced for the moment the line snaps.

**Aqua.** Kazumaaaa! It is still looking at me! Of course it is looking at me — I am the only divine light in this graveyard, so stop loitering! (Presses the stone angel's head to her burned arm, her light flaring against the far gate.)

**Megumin.** The thread is reeling, Kazuma! You have seconds, not scenery! (Her free hand cuts the air, the first syllable burning bright against the ash.)

**Darkness.** It turns from my shield... no — I will make it turn back, even if I must hurl myself into its reach! (Breathes in, braced low behind Chunchunmaru, voice fraying to a murmur.)


*Megumin sees.* The thread is taut now — not retreating, just coiling its slack, and I can feel the seconds burning in my raised hand. Aqua's light is stuttering against the gate, and her shouting is more frayed than theatrical. Darkness has gone quiet in that way that means she is about to do something reckless. Kazuma is still narrating when he should be moving. The ash drags at my knees like it knows what is waking.

*Aqua sees.* That wretch is still staring at me. I'm the only one doing the holy work while Kazuma dawdles—my arm burns where that thing touched, and the stone head is the only comfort. The ash pulls at my skirts, and that thread hums like it wants to yank me out of my own skin. I won't be dragged down into this filth. Not while I still have light to throw at it.

*Darkness sees.* The ash drags at my knees like a reminder of how heavy this world is, and that humming thread pulls my eyes to the gate. I want it to look at me instead. My shield is low, my blood is hot, and I can already feel the impact I am begging for — even if I have to throw myself into its reach to earn it.

*Aqua, privately.* I'll throw light at its feet first, not to kill it but to make it flinch before it can look higher, then walk into the circle and set my burned hand flat on the ash like it's already below me. If the thread yanks, I'll call it rising, not falling.

*Megumin, privately.* He's still narrating like the dark will wait for his summary, and Darkness is quiet like she's already volunteering. Fine—I'll let it coil until it leans in to take her, then detonate from inside the pull so the only thing it fastens on is the blast.


### 75

**Kazuma.** This is the same scene, everyone's here, the shade's about to snap toward Aqua at the gate. I'm already running to her — I keep going, closing that gap fast.

"Hold the gate, Aqua! I'm right behind you — just a few more strides!" I shout over the ash, pumping my legs harder, closing the distance between me and that trembling light.

The shade has stopped measuring. Its leading edge lifts clear of the ash, and the whole gray column holds there, balanced, while the thread draws so tight that the air between it and the gate seems to ring. Your last two strides land hard and clear, and Aqua's light catches you square, throwing the gate's shadow in bars across the ground behind her.

She is right there, trembling with the effort of the light, the stone angel's head pressed to her burned arm. Behind you, Megumin's syllable burns brighter than the gate-light. Darkness does not speak, only shifts her weight behind Chunchunmaru as the shade begins its slow, final lean.

**Aqua.** Kazumaaaa! I am not holding the gate like some hired guard — I am about to purify everything on the other side of it! So run faster, before that gray thing makes me do all the work while you loiter in the ash! (Pressing the stone angel's head to her burned arm, her light flaring against the far gate.)

**Megumin.** It is not retreating, Kazuma — it is coiling! The instant that thread springs for her light, I will spend my last dawn-fire on its spine and give this graveyard a crater to remember! (Holding her free hand raised, the first syllable burning bright against the ash.)

**Darkness.** Let it spring, then! I am already braced to be the first thing it strikes... so go, and do not look back at what it does to me! (Widening her stance behind Chunchunmaru, her voice fraying into a breathless murmur.)


*Darkness sees.* The shade hangs overhead, its leading edge lifting clear of ash while the thread sings taut between us. Aqua's light throws the gate's bars across the ground, and I feel the weight of Chunchunmaru in my hands, braced low. Kazuma's strides land hard behind me; Megumin's syllable burns hotter than the gate-light. Everything is balanced on the next instant — the shade coiling, not retreating, and I am already where it will strike.

*Aqua sees.* My arm is burning under the stone angel's head, and the light is eating all my strength while Kazuma is still scrabbling through the ash like he has all the time in the world. I can feel the gate thrumming through my palms, and that gray thing is just hanging there, coiling, waiting to spring at my light — of course it is, because I am the one doing everything again. Megumin is shouting about dawn-fire and craters, Darkness is muttering about throwing herself into it, and I am the one actually holding this gate shut. My light flared when I yelled, and the shadow of the gate stretched over the ground, and for one instant I felt the whole weight of the shrine behind me. If Kazuma does not get here right now, I swear I will purify him along with the shade.

*Megumin sees.* The ash is thick enough to taste, and my syllable already burns on my tongue like a promise. I see Kazuma's shadow stretched long by Aqua's light — he is almost there, almost past the gate's bars, but the thread between us sings with strain. The shade is not lunging; it is gathering itself, folding like a serpent before it strikes. Darkness is a statue behind Chunchunmaru, her weight set as if she means to be the first wave. And Aqua's glow is desperate, a trembling flare that smells of ozone and burnt cloth. My free hand aches to release the spell, but I know the exact heartbeat to wait for. The graveyard holds its breath with me.

*Darkness, privately.* It's going to land now, and I'm not raising the blade—I'm biting the inside of my cheek so the sound stays in my skull, where the light can't reach it. Let them see the shield catch the blow; I'll be under it, feeling the seam rip.


### 76

**Kazuma.** I throw the last of my weight into the strides, boots chewing through the ash, and reach the gate just as the thread goes taut. "I said hold it, didn't I? A goddess and a NEET can hold a gate against one gray lump." I plant myself beside her, grabbing the gate's iron bars with my free hand, bracing.

The taut thread does not snap. It bends the shade's whole height toward the gate like a man leaning over a table, and the ash between the graves rolls ahead of it in a slow gray swell. Aqua's light flares at your side and steadies for half a breath, bright enough to show the pale shapes beyond the bars drawing back — then it gutters again, and her weight sags an inch against the gate.

The moment balances there, held on her trembling arm and the bars under your hand. Megumin's first syllable is a live coal above the ash; Darkness has stopped breathing in little huffs and gone very still, the kind of still a shield makes before the blow finds it.

**Aqua.** Kazuma! You finally stopped loitering! Now hold the gate, hold the light, hold ME — my arm is about to fall off, and this gray thing will not stop staring at a goddess! (She presses the stone angel's head to her burned arm, her light flaring and stuttering against the iron.)

**Megumin.** The thread is taut, Kazuma! The instant it springs for her light, my Explosion will split the night and your eardrums both — so make her light worth the crater! (Her free hand cuts the air, the first syllable burning bright against the ash.)

**Darkness.** Let it spring toward the gate, then... I will be the wall it breaks against, and I will not be ignored a second time! (She widens her stance behind Chunchunmaru, her voice fraying into a breathless murmur.)


*Megumin sees.* My first syllable is already scorching the air, and the ash is rolling toward the bars like the whole graveyard is exhaling. Aqua's light keeps guttering and swelling, and I can smell burned skin under that holy stink. Darkness has gone preternaturally still behind me, and Chunchunmaru's weight is set like she means to become furniture. The thread is so taut I can feel it humming through the dust, and for one breath everything is balanced on Aqua's trembling arm and Kazuma's hands on the gate. I do not need to see the pale shapes; I can feel them drawing back, and I want the spring — I want the whole night to split open and prove that this waiting was worth the crater.

*Darkness sees.* Ash in my mouth, the gate shuddering under my grip, Aqua's light stuttering against my skin, and the shade's weight bending toward the opening I hold. I feel my own stillness—that quiet before impact—and the heat of Megumin's spell gathering at my back like a promise I am meant to receive.

*Aqua sees.* My arm is one long burn and I’m pouring light into an iron gate while the stone angel’s head presses cold against the wound—cold helps, but not enough. The ash is crawling toward the bars like the graveyard itself is leaning in to look at me, and I can feel the weight of that shade’s staring, gray and patient, as if I’m not a goddess at all. Megumin’s syllable is a live coal hanging in the air, and Darkness has gone so still that the only sound I can trust is my own light crackling. I hate being watched like this—being the thing the dark waits for—but I won’t let the light die, not with everyone braced around me. Kazuma moved. Finally. I need his hands on the gate and on my arm before my strength gives out entirely.

*Aqua, privately.* He's not moving toward the gate at all—he's coming for the arm that's about to go dark, and I won't make it into a lesson this time. I'll let him catch me under the light and call it bracing the door.

*Darkness, privately.* If Megumin's fire catches me from behind, it'll prove I was never just the wall—I was the thing they both fired through. I'll stay in the middle and let the two of them decide what's left.

*Megumin, privately.* I can feel the first syllable already burning my lips, and I'm done letting Kazuma's hands on the gate keep time. Let the dark lean in until it thinks it has her, then I'll give it the only name it gets to swallow.


### 77

**Kazuma.** The shade still hasn't sprung — just coiled, measuring, waiting. I keep my weight on the gate, but these three are up to something that's burning through time we don't have. Megumin's about to blow the whole graveyard, Darkness is ready to hurl herself into the thing, and Aqua's arm is shaking. Fine. Everyone's here, everyone's at the gate — this is the moment.

"Megumin, Darkness — keep that thing pinned to the far gate, don't let it spring." I shift my grip to Aqua's wrist. "Aqua, you with me? We go over this gate together, get inside the graveyard proper, and you purify every last gray wisp on the other side. That's the quest — purified, not blasted to cinders."

Your order never reaches its last word. The shade does not snap the thread; it commits to it, the whole gray column leaning past the point of balance and then pouring forward along the taut line as if the thread were a rail. The ash between the graves does not roll now — it lifts, a waist-high gray wave keeping pace with the thing's lower edge, and the iron bars of the gate begin to ring before anything touches them. Aqua's light flares white against the leading edge, and for half a breath you can see what the gray has been carrying: a long, wet-looking emptiness, a jawless shape that drinks the light rather than reflecting it.

Megumin's raised hand has stopped cutting the air. It hangs open, and the syllable that had been kindling behind her teeth is suddenly the brightest thing in the cemetery, a red-orange point that flattens the ash in a perfect circle around her feet. The heat arrives before the sound — a pressure that pushes your clothes flat and tries to peel your hand off the gate.

Then the night comes apart. The blast does not roar so much as hit, a column of light and dirt that takes the shade in its midsection and drives it down and away from the gate in a rolling curtain of ash and stone. The gate shudders against your shoulder, and Aqua's light at your side snaps to full, then gutters to a trembling point.

**Megumin.** It springs — then let it learn the weight of my name! EXPLOSION! (She releases the spell as the shade pours forward, and the red point detonates into a pillar of light and dirt.)

**Aqua.** Kazumaaaa! My ears! My dress! That maniac has buried a goddess in graveyard dirt! (She clings to the gate with one hand, light flaring and stuttering as the shockwave washes over her.)

**Darkness.** It was coming for the light... and she stole it from me... I was supposed to be the first thing it struck... (She staggers half a step forward through the rolling ash, blade low, voice fraying to a murmur.)


*Darkness sees.* The air turned solid before the sound did, and then the gray thing that should have reached me first was simply gone, torn sideways by Megumin's light. I felt the gate against my shoulder, Aqua's light stuttering beside me, and the ash clinging to my skin; my blade had not even lifted in time. It was beautiful, and it was stolen from me.

*Aqua sees.* The blast took the night and slammed it into me. Dirt in my mouth, in my sleeves, under my dress — my ears are ringing a stupid high note, and the light I'd been holding just guttered like a candle caught in a storm. I was the radiant one; I was the target; I was winning. Then that manic child turned the whole graveyard into a bomb, and now I'm the goddess who got buried up to her ankles in filth. The shade came for me, I know it did, and she stole that moment too. Darkness is over there sulking because she wasn't the one being eaten, which is exactly the kind of competition I do not need while I drown in ash.

*Megumin sees.* For a single heartbeat the column was all I was — the word and the fire and my name crushed together until the pillar reached the night and tore it open. Then the world came back in pieces: iron ringing in my teeth, dust tasting like defeat on my tongue, and the beautiful, punishing weight of having spent everything. My hand still floats where I left it, but the rest of my body has already agreed to fall. Aqua's screaming is a wet, distant thing; Darkness is alone in the ash with her blade and her regret. But I am the one who took the thing's shape and erased it. The cost is the proof — my knees are beginning to buckle, and I can feel the familiar temptation to let someone catch me. I won't ask. My name did the work; the price is mine, and I will pay it on the ground, grinning if I can manage it.

*Aqua, privately.* I'll keep the dirt in my mouth and not get up. Let him wade in and pull me out, because if I climb out on my own it's her mess, not my miracle.

*Darkness, privately.* I'll stay behind when they head back and dig my hand into the scorch until it stops being warm. Then I'll walk the treeline until the cold comes back—or until I admit it never wanted me at all.

*Megumin, privately.* I said I'd pay it on the ground, but the ground is just a direction and my grin isn't coming. If someone catches me, I'll call it the crater's last echo, not rescue.


## IV. The far gate

### 78

**Kazuma.** I tighten my grip on Aqua's wrist and nod toward the gate. "That's the plan. You purify, they pin it — I'll cover your back with Chunchunmaru. Ready?"

The name of the sword is still hanging in the air when the blast has its way with the world. The pillar climbs behind you, red at the root and black where the dirt crowns it, and the shockwave comes through the ash in one flat shove that slams the gate against its frame and sets every bar ringing. By the time the light rolls back, the shade is no longer pouring toward the bars. Its middle has been blown out into a ragged gray mouth, fire still showing through, and the whole column reels backward along its own thread toward the yew.

Megumin is not standing. She is down in the ash where the recoil threw her, flat with one arm flung out, smoke rising from her sleeve, and her face is still turned toward the crater as if the sight alone is holding her awake.

Your grip closes on Aqua's wrist, and her light steadies under your hand for the first time in minutes, bright enough to throw the gate's shadow straight back over your shoulder. But the sword you named is not at your side; Chunchunmaru is still low in Darkness's hand, its edge dull with ash, and Darkness has come another step forward through the settling gray as if the blast gave her the opening she was waiting for. Beyond the bars, the thread does not snap; it slacks and drags through the ash, coiling loosely behind the driven-back shade.

**Aqua.** Kazumaaaa! My ears are ringing, my dress is full of dirt, and my arm is about to fall off — and you are asking if a goddess is READY? Hold my wrist and do not let go, because I am about to make that gray thing wish it had never looked at me! (Her light flares as she presses the stone angel's head tighter against her burned arm.)

**Megumin.** The gray thing... has learned the weight of my name... If it rises again, Kazuma, you may carry me closer and I will whisper the rest of it into the wound... (Flat in the ash, smoke lifting from her sleeve, still watching the crater.)

**Darkness.** Chunchunmaru is already in my hand, Kazuma. I am the one who covers your back — so hold her light steady, and let that thing reach me first... let it... (She takes another step through the ash, blade low, her voice fraying.)


*Aqua sees.* Finally, Kazuma grabs me—my light stops shaking and I can feel the gate shadow at my back again, but my ears are screaming, my dress is full of grit, and my arm burns where I pressed the angel. That little explosion brat is sprawled in the ash like a discarded doll, smoke curling off her sleeve, yet she’s still staring at the hole she blew as if the sight alone gives her breath. And Darkness has my Chunchunmaru—my sword—walking ahead like she owns the moment, blade gray and dull, while the shade itself wobbles backward on its thread, loose and dragging instead of gone. They’re all playing hero around me, and I’m the one holding the light—again.

*Megumin sees.* The crater is still smoking, and the ash tastes like glory on my tongue. I am on the ground, of course — a perfect casting always collects its price, and I would not have it any other way. The gray thing reeled back with my name burned into its middle, and even in the dirt I can savor that. Kazuma is holding Aqua's wrist, and her light is steady now, brighter, as if my blast finally gave her something to stand on. Darkness still holds Chunchunmaru — my sword, though she walks forward with it as though the weight of it were her own reward. Let her take one more step; let the thread drag. It is my fire they are all walking through, and I can still whisper the rest of my name if the thing dares to rise again.

*Darkness sees.* Ash in my mouth, heat still on my face. The shade is broken back but not gone; its thread drags like a loose leash. Aqua's light is solid under Kazuma's hand, and I can feel Chunchunmaru's familiar weight in my own. Good. Let it come to me. Let it try the line I am holding.

*Darkness, privately.* The thread is slack because it's waiting for me to stop being the thing it pulls at and start pulling back. I'll wrap it twice around my wrist and walk the drag until the dark has to decide if I'm worth dragging under.


### 79

**Kazuma.** I pass Chunchunmaru back into my own grip and step up beside Aqua, blade low. "Purify it, Aqua. We've got your back — make that thing wish it had never crawled out."

The pillar folds in on itself, red to black, and the dirt comes down in a slow gray rain that clicks against the gate. What is left of the shade's middle is a ragged tunnel of paler ash where Megumin's spell went through, and the whole column sways backward toward the yew, not falling so much as unthreading. The line that had been tight between it and the gate now sags and drags through the ash, coiling behind it in loose, twitching loops.

Beside you, Aqua drags the stone angel's head off her burned arm and presses it flat to the iron bars. The light that had been guttering in her palm climbs her wrist and over the gate like water finding a channel, and for a moment the bars shine as though the whole cemetery had been turned to glass. She turns the light on the swaying shape, and the gray column drinks it. The brightness bends at the edges, thinning as it goes, and the wound in the thing's middle pulls the light inward the way a hole pulls smoke.

Darkness takes another step past you, Chunchunmaru's point scoring a clean line in the ash. The shade's upper reaches curl toward her, then snap back to the light at the gate, and the loose thread behind it begins to gather again, one coil over another, tightening slow.

**Aqua.** It's working! It's working! Behold — Sacred Turn Undead — and do not blink, Kazuma, because a goddess only performs this one for the truly doomed! (She presses the angel's head to the bars, light pouring through the iron in a white sheet.)

**Darkness.** The light draws it back... no, it draws the light... hold it steady, and let it choose me first... let it choose... (She widens her stance, blade low, voice fraying into a murmur.)

**Megumin.** It has a hole where its middle used to be, and still it stands. Kazuma! If it will not die to one Explosion, then finish what my name has opened! (Flat in the ash, smoke lifting from her sleeve, she points one trembling finger at the shade.)


*Megumin sees.* From flat on my back, the world is a haze of ash and stubborn light. I see the tunnel my spell carved—pale, ragged, real—and the shade still refusing to fall. Aqua's light pours across the gate, and the thing drinks it like a parched animal, yet the hole I made pulls the glow inward, a wound that hungers. Darkness plants herself between it and us, sword low, murmuring some noble nonsense about being chosen. My arm trembles where I point, smoke still rising from my sleeve. Kazuma's footstep shifts the ash near my head. I am spent, and still the world refuses to end neatly.

*Aqua sees.* Finally, Kazuma grabs me—my light stops shaking and I can feel the gate shadow at my back again, but my ears are screaming, my dress is full of grit, and my arm burns where I pressed the angel. That little explosion brat is sprawled in the ash like a discarded doll, smoke curling off her sleeve, yet she’s still staring at the hole she blew as if the sight alone gives her breath. And Darkness has my Chunchunmaru—my sword—walking ahead like she owns the moment, blade gray and dull, while the shade itself wobbles backward on its thread, loose and dragging instead of gone. They’re all playing hero around me, and I’m the one holding the light—again.

*Darkness sees.* Ash is gritty under my boots, and Chunchunmaru's point cuts its clean line through it — I count the drag as a promise. That column swaying toward the yew is unraveling, a loose gray thread looping behind it like rope gone slack, and Aqua's light is a white sheet washing over the bars until the whole cemetery looks poured from glass. The shade drinks it — bends the brightness at its edges and pulls the glow into the hole Megumin punched through its middle, the way a mouth pulls smoke. It curls toward me, and my skin rises, waiting for the reach, the scrape of it — but it snaps back to the gate, gathering its coils slow, and I widen my stance so it will see me standing ready. I want it to choose me. Let the light hold it, and let it take me first. Megumin's voice cuts flat through the smoke; Aqua's boast rings off the iron — and all I can feel is the line of ash I'm drawing, and the soft, hungry pull of that wound, and the weight of the blade that will not strike.

*Aqua, privately.* Good—let Darkness carry it like a prop. I’m the one lighting the whole damn stage; when the shadow drops, they’ll all have to turn around and see whose glow it died in.

*Darkness, privately.* It turned to me and still chose the gate — so I’m leaving the sword in the ash and going to stand where the iron was. If it comes back, it will find no gate but me, and I’ll learn whether that’s too much shield or not enough.

*Megumin, privately.* I wanted a full stop, but my blast just bored out a throat, and now the thing is drinking Aqua's light while Darkness volunteers to be its next swallow. If it won't end, I'll make it remember whose hunger made it—even from the ground.


### 80

**Kazuma.** I shift my grip on Chunchunmaru, standing shoulder to shoulder with Aqua as Megumin's blast still rings in my ears. "He's right behind you, Aqua. One more push — hit it with everything you've got while Darkness keeps it honest."

Darkness steps forward alongside me, blade low. "It wants to reach her first. Let it try."

The white sheet leaves the bars without heat and walks across the ash, and every flake that touches it turns first to fine white grit, then to nothing. Where the sheet meets the tunnel Megumin opened, the shade's middle begins to steam, paler threads peeling away from the wound like silk drawn from wet cloth. The column sways toward the yew, but its lower edge no longer lifts; the waist-high wave that had kept pace with it stalls and then curls up Darkness's greaves, gray as smoke around her knees.

Darkness steps deeper into it, blade low and one shoulder turned so the ash rides the seam of her armor. Her breath fogs once and is gone. The bars under your hand hum with the same note they caught when the shockwave hit, and Chunchunmaru's hilt has gone warm against your palm.

Megumin is still down in the ash, one trembling finger aimed at the wound, and she mouths something too low to hear over the ringing in your ears. The shade does not scream; it pulls, the sagging thread dragging a furrow through the gray as the light keeps eating it from the middle outward.

**Aqua.** Vanish, vanish, vanish! Sacred Turn Undead! The hole that explosion maniac tore in you is a door now — and a goddess is walking in! (Her light flares until the ash around the gate casts no shadow at all.)

**Darkness.** I am the wall, and the wall does not step aside... let it reach me... let every gray finger of it choose me first... (She widens her stance as the ash climbs her greaves.)

**Megumin.** It folds toward the yew, not away! Kazuma, my Explosion has opened its middle — do not let it crawl back into the roots! (She pushes up onto one elbow, her pointing hand shaking.)


*Aqua sees.* The ringing in my ears is a second hymn, and my light is finally big enough to make the world kneel. I feel the ash under my fingers turn to nothing where my radiance passes, and I taste the wine of my own power — Sacred Turn Undead! The shade's middle is an open door, and I am walking through it. The warmth in Chunchunmaru's hilt is like a small congregation pressing back at me, and the tunnel Megumin opened is a wound I am making sacred. I am not small here. I am not the useless one. The room is kneeling.

*Megumin sees.* Still flat in the ash, smoke lifting off my sleeve, I push up onto one elbow and keep my finger aimed at the pale tear my Explosion opened in the shade. That white sheet is steaming along the wound, pale threads peeling away like silk from wet cloth, and it sways toward the yew—but the lower edge has gone still, and the gray wave that was chasing us is curling up Darkness's greaves as she steps deeper with her blade low and one shoulder turned. The gate bars are humming that same note they caught after the shockwave, and Aqua's light is so bright the ash around the gate casts no shadow at all. The shade doesn't scream; it just pulls, dragging a furrow through the gray while the light eats it from the middle. I'm mouthing something, but the ringing in my ears is too loud for me to hear even myself.

*Darkness sees.* The cold gray is climbing my legs, and I'm the one between it and the light. Behind me the goddess is burning through its middle; Megumin's voice tells me where the wound is. The sword is warm in my hand. Good. Let it test itself on me.

*Aqua, privately.* Now I'll hold it—not the spell, but the way Kazuma's face has to turn toward the heat before he can call it a party trick. If I let the radiance drop too soon, I'll be back to being the one they haul out, so I'll walk slower through the open door and make the dark wait for me.


### 81

**Kazuma.** I keep a hand on Chunchunmaru but hold my ground at the gate, eyes on the hole Megumin opened. "It's pulling light in through the wound — that's the one opening it's trying to protect. Hit it through the middle, not the head, and it won't have time to crawl back to the roots."

I take half a step that puts the gate between me and the sagging thread, ready to grab Aqua's wrist if the shade lurches for her.

The half-step puts cold iron at your shoulder and the sagging gray thread on the other side of it. Aqua's light has stopped being a sheet; it pours into the wound Megumin opened and works there like water through a seam, widening the hole from within. The shade's middle comes apart in pale curls that lift and fail, and where the dragging thread passes, the ash behind it lies in a clean white furrow. The air over that furrow shimmers like heat over a hot road.

The column sways toward the yew but no longer has a middle to fold. Its lower edge stays low, and the waist-high wave climbs past Darkness's greaves to the dark joints of her knees. She does not retreat. For one heartbeat the whole column lurches toward the gate, not walking but leaning, as if the thread still owed it something. The bars ring. Your hand is already closing the distance to Aqua's wrist.

**Aqua.** Yes! YES! Look at it come apart! A goddess does not run from grave-smoke, Kazuma — she lights it from the inside and then makes it thank her for the mercy! (Her light flares until the ash around the gate casts no shadow at all.)

**Megumin.** The wound my Explosion opened is devouring your light and coming apart from within. Keep the stream on it, Aqua — if it leans away now, it only falls toward the yew! (Pushes up onto one elbow in the ash, her pointing hand shaking.)

**Darkness.** The ash has reached my knees... let it climb... every gray finger that tests me is one that does not test her... do not lower the light for my sake... (Steps deeper into the rising wave, blade low.)


*Aqua sees.* I feel the cold iron at my shoulder and see the shade's middle splitting under the light I pour into it — the ash falls away in a clean furrow, and the column lurches at the gate as if it still expects something from me. Darkness is letting the ash climb her knees, and Megumin is pushing herself up in the dirt, pointing at the wound she made. The whole thing is coming apart, and it feels like my light is finally being treated as it should be.

*Darkness sees.* The ash has climbed past my knees like a dozen small hands, each trying its grip on the greaves, and through the shudder of the column I feel a heat that is not fire — it is the light tearing the shade open, and it lifts the hair on my arms. My own blade feels heavy, pointless, a weight I am grateful to carry. Every cold finger that pulls at the joints of my armor is a reminder that I am still the one standing between it and her. The bars rang once; I want to hear that again.

*Megumin sees.* The ash is in my nose and the cold of the blade’s shadow on my shoulder, and through the smoke I watch my Explosion’s wound working like a mouth that has found water — Aqua’s light pours into the gap I made and the column comes apart in pale curls, leaving a white furrow that shimmers like the road after rain. My hand is shaking because I am at the end of myself, but the end of myself is exactly where the creed begins: the doom opened the way, and the way is closing around the shade, not around us. The column still leans toward the yew as if it could crawl back into its roots, but there is no middle left to fold. I want to laugh. I want to be carried. I want this to be the part they remember — not the collapse, but the hole I made in the world that let the light in.

*Aqua, privately.* It split because I touched it, not because the wall or her blast finished it, and I won't let the light stop while it's still lurching—if it expects something from me, the last thing it sees can be exactly who sent it out.

*Darkness, privately.* That ring wasn't warning me off—it was measuring how far I'd go. I'll give it the next step, then another, and if his hand closes on my arm I'll pretend it's the shadow's grip I wanted.

*Megumin, privately.* I opened it and she filled it, and for once I don’t want to pull my name back before hers touches it. Let him carry me if he wants; the hole I tore in the dark still has my teeth in it.


### 82

**Kazuma.** I close the distance to Aqua's wrist and keep her light trained on the hole. "That's it — feed the wound, don't fight the whole thing. Megumin, if you've got anything left, put it through the middle again. Darkness, hold that line."

The gate's iron is cold against your shoulder, and through it the sagging thread begins to hiss as Aqua's light follows it down. Your hand closes over the knob of her wrist, and for one instant her whole arm jumps — the light bucks and throws a long white shadow across the graves — then you hold it, and the stream settles back into the wound Megumin opened. The hole is no longer a seam but a mouth, and it drinks the light down into its own middle, coming apart in soft pale curls that turn to grit and then to nothing.

The shade leans away from the gate, not falling but unspooling toward the yew, and its lower half drags along the ground like a cloak grown too heavy to lift. The ash wave climbs Darkness past her knees, gray fingers folding over the plates of her thigh; she does not retreat. Where the dragging thread passes, the ash behind it lies in a clean white furrow, and the whole column begins to thin at the waist, upper and lower halves twisting against each other.

The light widens the hole until you can see the far side of the cemetery through it — a strip of crooked headstones, then the black bole of the yew. For one held breath the shade hangs between two shapes, neither falling nor whole. Then the lower mass tears free of the middle with a sound like wet sailcloth giving way, and the column above it whips toward the roots, hauling the last of the thread behind it.

**Aqua.** K-Kazuma! A goddess's wrist is not a handle! ...Still, do not let go — the light is pouring straight through that hole, and I am not about to waste the one moment the whole graveyard is watching! (Her burned arm shakes, but she keeps the stream aimed into the wound.)

**Megumin.** I have nothing left to cast — my one permitted Explosion is already burning inside that wound! A Crimson Demon is not a firework you light twice, so be the blade that finishes what my name tore open, Kazuma! (Flat in the ash, she drags herself up onto both elbows, pointing with a trembling hand.)

**Darkness.** The ash is at my thighs... let it climb... every gray finger that closes on me is one less reaching for her... do not lower the light for my sake... I can bear all of it... (She widens her stance, blade low, voice fraying into a murmur as the wave climbs her armor.)


*Aqua sees.* The shade is coming apart at the seam — one moment a column, the next a dragged shroud pouring into the yew's roots — and the hole it left is spilling daylight through the whole graveyard. Darkness is letting the ash swallow her to the thigh, Megumin is flat on the ground with nothing left but pointing, and Kazuma is gripping my wrist like I'm a rope instead of a goddess. My arm is burning, but the light is mine, and I am not letting one drop of this moment go to waste.

*Megumin sees.* The ash is in my nose and the cold of the blade’s shadow on my shoulder, and through the smoke I watch my Explosion’s wound working like a mouth that has found water — Aqua’s light pours into the gap I made and the column comes apart in pale curls, leaving a white furrow that shimmers like the road after rain. My hand is shaking because I am at the end of myself, but the end of myself is exactly where the creed begins: the doom opened the way, and the way is closing around the shade, not around us. The column still leans toward the yew as if it could crawl back into its roots, but there is no middle left to fold. I want to laugh. I want to be carried. I want this to be the part they remember — not the collapse, but the hole I made in the world that let the light in.

*Darkness sees.* The ash has my legs, and I feel every gray finger as if it were a hand begging me to stay. The shade is breaking apart around me, but the hole of light keeps widening, and through the pain and the weight I hear Aqua and Megumin calling out past me — as if I were already a wall they could shout over. I will be that wall. Let it climb. I can bear all of it, if it means she reaches the light.

*Aqua, privately.* The burn can finally be the proof instead of the wound—I'll keep the light standing out of my arm until Kazuma has to hold me like a candlestick, not a rope. Let the hole close; I want them to trace the daylight back to my hand, not to the yew.

*Darkness, privately.* The only way to stay the wall is to stop flinching when it takes a little more of the part of me that came here wanting to kneel—so I uncurl my fingers in the ash and let the gray have my hands.

*Megumin, privately.* I'll let him lift me, not as rescue—I want the vantage. If the furrow is already closing, then I have to watch it while it still looks like a road, not after they've turned it into a story.


### 83

**Kazuma.** The light has found the hole, and the shade is already coming apart from the middle — but it's not dead yet. It's dragging toward the yew, lower half still heavy in the ash. Aqua's wrist is in my grip, the stream steady. Darkness is holding the line. Megumin is down but still aiming.

The hole is widening. It's working. But I feel it — this thing still owes us a fight before it dies.

The dragging stops all at once, and for one held second the whole shape hangs torn open above the ash, Aqua's light working inside the wound like a slow white current. Then the lower half bunches, finds something to push against beneath the ash, and rises.

The ash around Darkness's legs lets go with a sound like a wet inhale, and the freed mass swings over the gate in one broad gray arm. It does not strike. It reaches past you toward the light, toward the seam burning in its own middle, and Darkness moves before the arm has finished its arc.

Her shoulder takes it. The blow folds her forward over the gate, and the gray arm wraps once around her breastplate and shield arm, tight enough that her greave scrapes a bright line down the iron; but she does not fall. Her gauntlet locks onto the bar, and her whole body sets against the pull.

Behind you, the yew's bare roots slide black through the ash, and the wound in the shade's middle narrows by a handspan, then another. The light inside it dims to the color of steam. Megumin claws up the gatepost at your back, her pointing arm shaking, and the gray arm around Darkness creaks as it squeezes.

**Aqua.** It's closing the door! Kazuma, it's closing the door and your goddess is still inside it — don't let go of my wrist, and don't you dare let it shut on my light! (Her light sputters and bends, and she leans her whole weight against your grip.)

**Darkness.** Yes... yes, take the arm... take all of it... the wall does not mind... but Kazuma — the wound is closing; strike while it spends itself on me... (She speaks through gritted teeth as the gray arm constricts her breastplate.)

**Megumin.** It is folding the wound shut like a collapsing star! A Crimson Demon knows a dying star when she sees one, and this one wants company — pierce the middle before it becomes a fist! (She drags herself upright against the gatepost, one trembling hand aimed at the narrowing hole.)


*Megumin sees.* Ash grit under my fingers, the gatepost cold through my sleeve. My arm shakes but I aim it true — a wound folding shut is a dying star, and I have studied dying stars all my life. The light inside is going the color of spent steam, and Darkness is wrapped in that gray arm like a vice around a prayer. The creak is the sound of a spell refusing to be cast. My legs will not carry me, but my curse is still aimed at that shrinking heart of dark.

*Aqua sees.* The light in me is folding shut, and I can feel the seam pinching like a horizon slamming. Darkness is letting that gray thing crush her—she's using herself as bait, the madwoman—and Megumin is screaming about dying stars from the gatepost. I'm gripping Kazuma's wrist so hard my knuckles burn, because if that light closes, I'm not a goddess in a temple, I'm just a cold name in ash.

*Darkness sees.* The ash-arm has me — shoulder, breastplate, shield arm — and the gate is cold iron under my gauntlet. I feel the squeeze, the scrape of my greave, the pull that wants to drag me forward. The light in the seam is failing; I can see it narrowing, hear Aqua shouting past me, hear Megumin calling it a collapsing star. I hold the bar with my whole body and let it spend itself on me, braced for the strike to land.

*Aqua, privately.* No—Darkness doesn't get to be the martyr while my light is still warm; if the seam wants to shut, I'll bite it open on Kazuma's wrist and feed the gray thing every candle I have left, then make them all look at whose fire it was.

*Darkness, privately.* The failing seam is worse than the squeeze—if it closes, I'll only be a body in the dark, not a wall. I shove up into the pressure so the last thing the light sees is me refusing to fold.

*Megumin, privately.* It’s folding shut to keep her as the one thing I didn’t burn—so I’ll aim at the seam, not the heart, and make the dark let go or close around my handprint instead of her outline.


### 84

**Kazuma.** I keep hold of Aqua's wrist, pull her forward a step, and bring Chunchunmaru up in my other hand — ready to support what Darkness is giving. "Megumin, you called it. It's running for the yew. It doesn't get to reach those roots."

The ash gives under your boot as you pull Aqua forward a step, and her light comes with her in a long unsteady arc that catches the inside of the wound and holds there, bright for one more second. Chunchunmaru rises in your other hand until its edge finds the gray daylight, and the point hangs level with the seam where the shade is folding itself shut.

The gray arm over the gate is not a limb so much as a current that has chosen a shape. It crosses the iron at chest height and closes around Darkness with the slow certainty of a river finding its channel, and the ash climbing her thighs has gone still, as though the whole mass is holding its breath to watch. Her breastplate complains under the pressure, a low tin sound swallowed by the ash.

Above it the upper column keeps folding, drawing the pale wound down to a slit, and inside that slit Aqua's light has thinned to steam. Megumin's pointing hand trembles against the gatepost, her aim fixed on the narrowing seam.

**Aqua.** It runs because a goddess is inside it! Keep hold of my wrist, Kazuma — pull me in, and I'll make the dark thank me from the inside while you chop it into grave-dust! (Her light sputters and bends as she leans her whole weight against his grip.)

**Megumin.** The slit is closing! A dying star does not flinch, but you are not a star, Kazuma — one thrust, and make it the one they carve on the guild door! (She pushes off the gatepost, her pointing hand shaking.)

**Darkness.** The arm... it means to drag me into the roots with it... d-do not wait for me... a Crusader is the anvil, so strike, Kazuma, strike while it spends itself on me! (Her voice frays as the gray arm tightens around her breastplate.)


*Aqua sees.* My own light is thinning into steam inside that closing wound, and that filthy arm has Darkness pinned like a doll — I can hear her tin armor groaning. Kazuma still has my wrist, thank whatever is left, and the whole gray mass is holding still to watch me. It runs because I'm inside it, because I'm a goddess, and it should thank me while I make it choke from within — but the slit is collapsing, and Megumin keeps screaming about dying stars, and I am the one being dragged into the dark. That is not how this was supposed to go at all.

*Megumin sees.* My knees scrape the gatepost; the ash is cold and it is hungry. My arm is up—trembling, a death rattle of a gesture—and I can feel the point of my own resolve narrowing like the slit I am aimed at. Chunchunmaru catches the gray light, and I think: this is the frame of a legend, if only they would move. But they are all held by the dark in their own ways—one by the wrist, one by the breast, one by the grace of light that is gutter and steam. The gray is patient. I am not. The slit is a keyhole to nowhere, and I want to be remembered aiming through it. I feel the line from my finger to the wound, and it is thin as thread, but it is mine. Even as my hand shakes, I want to be the one who casts the shadow of resolve across this moment. Almost, almost—I can hear the applause that will never come, and it is enough to keep me standing.

*Darkness sees.* The iron bites into my back and the ash has gone quiet, as if the whole world is waiting to see me hold. Their voices come to me from far away, but I know what they need: a place to strike. I can be that place. I want to be that place.

*Aqua, privately.* If it wants me inside, then fine—I'll stop pulling against Kazuma's hand and let the seam take me already glowing, so when it closes it'll be around the one thing in this world that can drown it from the inside.

*Megumin, privately.* The applause was enough to keep me standing, not to make the dark burn—so I'll trade the standing for the cast. Let the gray keep its patience; I'm done aiming through a keyhole, I'll put my whole arm through.


### 85

**Kazuma.** I drive Chunchunmaru's point into the furrow where the shade's thread is still trying to reach the yew, cutting the last gray tendon before it can dig into those roots.

The point goes in and meets resistance, not the loose give of ash but something rooted and gristly beneath it, and for a beat the whole gray length of the thread stiffens against the blade. Then it parts. The severed end whips out of the furrow like a snapped cable, spraying pale dust across the gate, and where it had been digging the ground closes with a soft, wet sound, ash sliding into the empty seam.

Above the yew the shade convulses, the torn middle still open and swimming with Aqua's light. The gray arm over the gate jerks once, tightens around Darkness's breastplate with a long metallic creak, and does not let go. The thread no longer pulls toward the roots. It hangs, severed, from the mass that is still trying to fold itself shut.

Aqua's light bends where your grip holds her wrist, then pours back into the wound as the hole widens, its edges coming apart in white curls that rise and fail like smoke through the yew's bare branches.

**Aqua.** Ha! Severed! That is what happens when a goddess is holding your wrist! Now pull me closer, Kazuma, and I will make the whole graveyard thank me from the inside! (Her light flares and bends as she leans into the open wound.)

**Megumin.** The last thread to the roots is cut! It is now only a dying star with no earth to swallow it — do not lower the blade, Kazuma! Let Explosion's ember burn it out from the middle! (She drags herself up against the gatepost, pointing with a shaking hand at the widening hole.)

**Darkness.** It... it squeezed harder when the thread parted... as if it means to drag me down with it... d-do not loosen the light for my sake... I can still bear every gray finger... (Her voice frays as the gray arm creaks around her breastplate.)


*Darkness sees.* The gray arm creaks around me, and I feel the pressure as a promise—tightening, claiming, holding me exactly where I am supposed to be. When the thread parted, something in the mass flinched, and it gripped me harder, as if I were the last thing it could keep. I heard Aqua's triumph and Megumin's command, but mostly I felt the metal of my breastplate bending against their rescue, and I wanted to say: do not save me from this. I can bear it. I want to bear it.

*Aqua sees.* My own light is thinning into steam inside that closing wound, and that filthy arm has Darkness pinned like a doll — I can hear her tin armor groaning. Kazuma still has my wrist, thank whatever is left, and the whole gray mass is holding still to watch me. It runs because I'm inside it, because I'm a goddess, and it should thank me while I make it choke from within — but the slit is collapsing, and Megumin keeps screaming about dying stars, and I am the one being dragged into the dark. That is not how this was supposed to go at all.

*Megumin sees.* I drag myself up against the gatepost, ribs burning, and watch the severed thread whip out of the furrow like a snapped cable — pale dust sprays, and the ground closes over the seam with that soft, wet sound. The hole in the shade is a torn mouth swimming with Aqua's light, its edges peeling apart in white curls that rise and fail like smoke through the bare yew. Darkness's voice comes to me strained, wrapped in the long metallic creak of that gray arm still clamped around her breastplate — it hasn't let go. My finger shakes as I point at the widening wound, and I feel the last ember of my explosion still smoldering in my chest, waiting to be spent. The root is cut. Now it is only a dying star, and I want to burn it out from the middle.

*Darkness, privately.* Their hands will find my straps and pull before the gray lets go, so I’ll go heavy—not to fight them, but to make them feel the weight of what they’re taking from me.


### 86

**Kazuma.** The ash slides off my boot as I yank the blade free of the severed furrow. I pull Aqua's wrist forward a step, and with the point still wet, I drive Chunchunmaru up into the widening wound — straight into the middle of the seam where her light is thinning to steam.

"Finish it from the inside, Aqua! I've got it pinned — now tell the dark you're here!"

The blade goes in and the shade stops folding all at once, as if Chunchunmaru has driven a nail through the center of a muscle. The seam freezes around the steel, and through the gap comes Aqua's light, no longer thin steam but a white current that floods the torn middle and turns the gray to something translucent from the inside.

For one second the whole shape stands lit like a paper lantern with a hole torn in it. Then the arm over the gate convulses, and the fingers around Darkness's breastplate lose their outline at the edges. The grip loosens not finger by finger but all at once, the arm unraveling into a rush of pale ash that spills down her pauldrons and across the gate bars.

The lower half heaves once in the ash and then collapses inward, folding not shut but apart, and the wound blows wide with light. A long exhale moves over the cemetery, cold and then gone, and the shape comes down as dust — a gray curtain settling over the furrow, the roots, the quiet patch of ground beneath the yew. Chunchunmaru pulls free with a whisper, and Aqua's light gutters low but does not go out.

**Aqua.** There! THERE it is! That is what happens when a goddess gets inside a thing — it dies of gratitude, Kazuma, and I felt every last speck of it! Now say I was magnificent, or I will cry right here in front of the dead! (She sags against your grip, the angel's head still clamped to her burned arm, her light sputtering like a candle in a draft.)

**Megumin.** The star has gone out. But a Crimson Demon does not cheer while the ash still holds its shape — do not sheathe the blade, Kazuma! Let the ground settle, or the dark will remember that it had hands. (She sways against the gatepost, her pointing hand still raised at the settling dust.)

**Darkness.** It... it let go all at once... as if it spent itself... My breastplate holds, and yet my duty feels incomplete... Kazuma, that pressure — I could have borne it to the very last gray finger... (She braces herself upright with the flat of her sword, gray ash sliding from her shoulders, her voice fraying into a breathless murmur.)


*Megumin sees.* The ash settled like a curtain, but I kept my hand raised. A star does not die twice; the shape was gone, yet the ground still remembered its weight. Hunger for the sight of my own spell — I felt it twitch behind my ribs, watching that pale collapse, wanting to add my crimson to the gray. Only the ache in my legs and the gatepost at my back told me I had spent nothing at all this time. That is the strange part: to watch something end and know I was not the one who ended it.

*Darkness sees.* The grip that held me vanished before I could prove I could endure it. All that weight, all that attention, dissolved into ash against my plate—and the silence afterward feels heavier than the pressure ever did. I am left standing, intact, and somehow that is the cruelest part: no strain to bear, no line to hold, only this hollow ache of a duty that never got to finish.

*Aqua sees.* I felt it pour into me—every gray fleck of it, every last speck of relief, like the whole thing finally understood who I am. And I let it happen. My light went thin and shaky, but I didn't stop glowing. The cold on my arm is still there, and the head is still clamped to me, but I'm not letting go. I want to be praised. I need Kazuma to say it loud, before the ash settles and the moment forgets me. Megumin's still tense, Darkness is still hungry for more pain, and I'm standing between them with a dead angel stuck to me, feeling like I just won a war and a funeral at the same time. I'm magnificent. I know I am. Say it. Say it before I cry in front of these dead people.

*Aqua, privately.* The cold is sliding between my ribs now, so I can't be the one to say it first—if I do it's just a girl touching her own wound. If Kazuma won't announce the win before he checks my arm, I'll make him carry the head into the guild and tell the living what the graveyard saw.

*Darkness, privately.* I'll kneel where it let go and press my bare hand into the scorch until the heat gives me back a shape—if the dark won't finish the hold, I'll at least walk out wearing its mark instead of this clean, unbearable armor.

*Megumin, privately.* That stillness just proved the ending can belong to someone else, and I won't let it. I'll peel myself off the gatepost before anyone hangs a plaque for the pale collapse, find one bare patch, and spend the hunger they all think I already used.


### 87

**Kazuma.** I feel the steel bite, and the seam freezes wide open around Chunchunmaru's length, spilling bright light down across the ash.

"It's holding! Aqua, pour everything you've got into that hole — and you, Megumin, keep your hand up. When it stops folding, you get your pretty explosion to bury whatever's left."

The steel bites deep and holds. What spills through the seam is no longer thin steam but a white current, and it floods the shade from the inside so completely that the gray folds go translucent, their shape showing through like veins in a leaf held up to the light.

Aqua leans her whole weight into your grip and does not stop. Her light pours in until the folded shell begins to unstitch at its edges, threads peeling away from the blade and lifting as pale cinders. The slack gray arm that had been wrapped around Darkness unravels last, unspooling into the air and drifting apart before it reaches the ground.

Beneath the yew the ash stops ringing. Megumin stands against the gatepost with her hand still raised, though the gesture is now more proclamation than aim, and her knees tremble with the weight of the dawn spell already spent.

**Aqua.** Ha! It is coming apart! That is what happens when a goddess pours herself into a wound — now say I was magnificent, Kazuma, and say it before the ash finishes falling, because my arm is about to give out! (She sags against your grip, the stone angel's head still clamped to her burned arm as her light sputters at the edges.)

**Megumin.** Keep my hand up? Fool, this hand has not lowered since the gray thing first folded! But I have already spent today's Explosion on the dawn — bury this husk with Aqua's light, or carry me home and argue about it later! (She sways against the gatepost, her raised hand trembling with more than dramatic emphasis.)

**Darkness.** The arm... it came apart around me, and I did not get to feel the last gray finger tighten... Still, if the shade leaves any shape behind, plant me in front of it again. A Crusader's duty is not finished while any gray thing still holds its breath. (She braces herself upright with the flat of Chunchunmaru, gray ash sliding from her shoulders as she watches the unraveling shell.)


*Aqua sees.* I poured myself out again — my light, my holy radiance, my very divinity — and of course it worked, because that is what happens when I actually bother. But no one is kneeling, no one is chanting my name, and my arm is burning and trembling so badly I can barely hold this stone head. I feel magnificent and furious at the same time, which is the most goddess-like feeling there is. I need praise now, immediately, before I collapse — and Megumin is stealing the drama with her spent Explosion, and Darkness is sulking about not getting hurt enough, and the ash is still falling. Why is it always me doing the work and everyone else standing around looking tragic?

*Megumin sees.* My arm stays aloft because I decree it, not because it obeys me—every tendon hums with the echo of sunrise fire already gone. Aqua's light is a vulgar, flooding thing, but I watch it do what my Explosion could not: unmake the gray husk thread by thread, like a poem dissolving back into silence. The ash falls, and Darkness rises from it still chattering about duty, and I am leaning on a gatepost with nothing left but the shape of a spell I already spent. That shape is mine. Let the goddess have her unraveling; I had the dawn. The tremor in my knees is the only honest part of me left, and even that I will dress up as emphasis.

*Darkness sees.* The light came through me as much as around me — hot, white, and impersonal, and it did not wait for my permission. I felt the gray arm begin to loosen, then spool away, and I hated that I had not felt the final grip. The ash in my hair is still warm. I am standing, and I am not sure whether that is victory or interruption.

*Aqua, privately.* I'm done waiting for them to say it—I'll take Kazuma's coat, the first full cup out of Darkness's hand, and walk back first with the head under my arm like the only one who didn't just stand there.

*Darkness, privately.* The light finished it before I could, so I’ll carry the ash and the heat like a confession no one gets to wash off. If they ask if I’m hurt, I’ll say it’s only the cold and let them believe it.

*Megumin, privately.* She got the last light, but I put the crack there, so I'll drag my spent legs to the crater's edge and leave my handprint in the ash before they can say Aqua ended it.


### 88

**Kazuma.** I drag the blade free with a whisper and it comes out clean, pale cinders sliding off the steel.

"Good work, all of you. Now let's make sure nothing crawls back out of that furrow before we call this done."

I step forward, kicking through the gray dust to the shallow trench the thread had been digging, and crouch to check the seam in the ground.

You step through the gray drift, and the cinders turn under your boot with a sound like dry snow. The trench is shallower than it looked from the gate, a raw black slot no wider than a hand, its sides glazed where the thread had been worrying at the earth. You crouch, and the last heat comes up out of it — not hot, but clean, the way stone smells after lightning.

At the bottom the furrow is closed. The place where Chunchunmaru went in is a neat pale seam, already crumbling at its edges, and nothing moves behind it. A little ash slides down the slope and settles, and the ground does not open to take it.

Behind you, the shade's last shell folds into a loose gray stain over the gate and comes apart in the wind, too spent to hold a shape. Darkness still stands in the middle of it, braced on the flat of her sword, ash sliding from her pauldrons in slow white runnels. Megumin has not lowered her hand. Aqua is listing sideways with the stone angel's head clamped to her burned arm, her light fizzing at the fingertips like a taper that cannot decide whether to go out.

**Aqua.** Kazuma, stop staring into that hole and say I was magnificent! The dead are quiet, the dirt is closed, and my arm is about to fall off — if you make me hold this pose one more second, I'll cry all the way back to the guild! (She sags against the angel's head, her light sputtering.)

**Megumin.** The ash is still falling, Kazuma. A genius does not sheathe her blade until the last cinder has touched the ground. (She sways against the gatepost but keeps her hand raised.)

**Darkness.** If the ground opens again... put me in front of it. I can still bear every last gray finger. (She braces herself upright, breathless.)


*Aqua sees.* They're all still ignoring me. Kazuma's crouched over that hole like it matters more than my burning arm, Megumin won't lower her hand because of some ash, and Darkness is begging for another threat that's already gone. The stone head is heavy, my light keeps sputtering, and nobody has said a single word about how magnificent I was.

*Megumin sees.* The ash is still falling. That is the first thing I notice, the only thing that matters—the proof of my craft, still settling over this battlefield like a benediction. Kazuma crouches at the sealed furrow, inspecting my handiwork as if he could find fault in it, but the ground is closed, the dead are quiet. The air tastes of lightning and clean stone, the smell of a spell that did exactly what it was meant to do. I can feel the weight of my staff in my hand, the deep ache in my arm that tells me I have spent everything, and that is good. That is correct. Behind me, Darkness is still braced for a battle that is already over, and Aqua is demanding praise with her light guttering like a candle in a draft. They are both so loud, but I am the one still holding my pose, because genius does not bow to fatigue. Though I will admit, if Kazuma does not come and carry me soon, I may have to reconsider my definition of 'magnificent.'

*Darkness sees.* The trench has sealed itself—a pale, crumbling seam where Chunchunmaru went in. The last heat rising from it feels clean, not angry, like stone after lightning. I feel the ash leaving my shoulders and the flat of my sword taking my weight; my arms are still braced, because part of me is still waiting for the ground to split and give me something to bear. Kazuma crouches over it, Aqua is listing with the angel head against her arm, Megumin still holds her hand raised. The moment is done, but I haven't stopped being ready.

*Aqua, privately.* I'll put the head down and walk back alone; if the light goes out on the road, that's mine, not theirs. Kazuma can stay crouched over the hole until he notices the only thing still glowing is already halfway to a drink.

*Darkness, privately.* The clean heat is worse than a wound—it means the trench closed without needing me. I’ll stay braced long enough for them to call it discipline, so no one sees I’m still waiting for the seam to split again.

*Megumin, privately.* I'll give him until the ash stops falling; then I'll take one step on my own, not because I can, but because a carried genius is a line and a walking collapse is a legend.


### 89

**Kazuma.** "Chunchunmaru came out clean, the seam's closed, and the ash is going quiet." I sheathe the blade and look back at them. "Good work. Now let's find that origin thread before it digs another grave — Aqua, you first, you reek of it. Which way?"

I turn to her, hand already pointing at whatever direction feels most tainted.

Aqua sways on her feet and lets the angel's head dip toward the lane, and for one long second her light bends that way like wheat before a wind, pale and thin at the edges. The taint behind the far gate does not answer. It simply waits, cold as the underside of a stone.

**Aqua.** The far gate, obviously! It is pooling back there like cheap wine in a cracked cup — wet, cold, and entirely your fault for making a goddess walk toward it! And I still have not been told I was magnificent! (She stumbles a step, the stone angel's head dipping toward the lane as her light gutters.)

**Megumin.** The ash beyond the yew refuses to rise, which means the air there is already spoken for. If we go, we go with Aqua's light held low and my hand free — a Crimson Demon does not walk into a held breath unprepared. (She pushes off the gatepost, swaying once before she finds her balance.)

**Darkness.** Then let me take the first step. If another gray thing waits behind that gate, I would have it spend itself against my shield before it so much as breathes on Aqua's working arm. (She lifts Chunchunmaru and turns toward the far gate, breath coming short and eager.)


*Aqua sees.* The air tastes like ash and that damp cellar smell that clings to my robes no matter how many times I purify them. My arm is a dead weight, my light keeps flickering like a cheap candle, and everyone is treating this like a poem instead of a job. Kazuma is still staring at that hole as if it owes him money; Megumin is doing her dramatic pose like the ash cares; and Darkness is practically salivating at the thought of another monster touching her. The far gate sits there, cold and patient, like it knows I'm the only one who can actually fix this and it's enjoying watching me stumble. No one has said I was magnificent. That's the real wound.

*Megumin sees.* The ash falls like a curtain that refuses to finish. Aqua's light is a candle drowning in its own wax — she wobbles, and I can smell the damp of the stone angel where her hand slips. The far gate squats in the cold like a mouth held shut; the taint behind it does not breathe, it waits. Darkness turns toward it with that hungry steadiness, Chunchunmaru glinting as if the blade itself is licking its lips. My own limbs are lead, but the pose holds because it must. The air beyond the yew is a held breath, and I will not be the one to let it out early.

*Darkness sees.* The ash is still in the air, and the two of them are spent—Aqua's glow bends and thins, Megumin holds her pose out of pride. The far gate is a cold weight in the dark, waiting. My own breath comes short, and that eager ache in my chest is answer enough. I want to be the one in front of it.

*Aqua, privately.* They won't say it, so I'll make them feel the difference—I'll pull the light back into my chest until the gate leans toward me and Kazuma has to call my name just to keep the dark from swallowing him.

*Darkness, privately.* Let them be spent; I’ll be the one still walking. The far gate is waiting, and if I reach it before Aqua’s glow thickens again, the dark won’t have to ask which of us came to be opened.

*Megumin, privately.* The ash not finishing means the dark hasn’t chosen an ending—it’s waiting for one of us to volunteer, and Darkness is already halfway to offering her pulse. I’ll give it my voice instead of her swallow; if the air is a held breath, I’ll turn it into the first syllable of the blast and let the gate choke on it.


### 90

**Kazuma.** Everyone's on-stage with me here. No off-stage storyline. Per my brief, I should split once — send Aqua only to the cemetery to purify the undead. But wait — we're already AT the cemetery gate. The scene title says "Axel Town — The Adventurer's Guild," but the recent narration has us at the yew, the far gate, the trench. We're at the cemetery right now, mid-purification.

Hmm. Let me reconsider. My brief says: "If everyone who matters is on-stage with you (no off-stage storyline) — split once, this turn, then stop splitting. Send Aqua only to Axel's cemetery to purify the undead..." But we're already at the cemetery. That fork instruction assumes we're at the guild. Sending Aqua to the cemetery when we're standing in it would be absurd.

Let me re-read the brief more carefully. The fork instruction wants one companion sent off with a named place and purpose, and I don't go with them. But we're already at the cemetery, everyone together, mid-cleanse. Sending Aqua to the cemetery "to purify" when we're literally standing in it would be nonsense.

Given we're already at the cemetery with everyone present and the fight's winding down, the sensible continuation is the "After you have reunited" state — but we haven't split yet. However, forcing a split now would be artificial and break character (sending Aqua to the place she's already standing).

The brief also says "Do not linger while a fork is live" and "no new send-offs." Given the party is fully together at the cemetery after the fight, the cleanest read is that the engine intends the split to happen. But it's physically incoherent here.

I think the practical call: continue the scene naturally. We're at the cemetery, job's largely done. The fork mechanic can't sensibly trigger here. I'll follow the story, staying together — which aligns with "After you have reunited: stay together for one full beat."

I should just continue my previous action: pointing toward whatever direction feels most tainted, pushing to find the origin thread. Let me respond in character.

"I don't have to point — the stink's coming from the back wall, where the old collapsed vault sits under the ivy. That's the origin thread. Aqua, you're leading that charge since you can smell it better than I can; Megumin, keep your hand up in case something bubbles; Darkness, you're on the front line and I know you're dying to be first into a cramped crypt."

The three of them move as one ragged line, Darkness's breath short and eager, Aqua's sandal catching on every second stone, Megumin walking with the careful stride of someone refusing to be carried. The ash behind them settles at last, and the trench stays shut.

**Darkness.** Stay behind me. If anything waits beyond that gate, I want its first breath spent against my shield, not on Aqua's working arm. (She lifts Chunchunmaru low and leads with the flat of the blade, ash sliding from her pauldrons as she steps toward the far gate.)

**Aqua.** I still haven't been told I was magnificent! A goddess walks into the wet dark with a stone head under her burned arm and all she gets is 'stay behind me'? This is your fault, Kazuma — every cold step of it! (She stumbles after Darkness, the angel's head dipping until its stone cheek smears a pale streak through the ash.)

**Megumin.** Hold the light low, Aqua. If the air beyond the gate is already spoken for, then I would rather not announce our arrival by waving your divinity in its face. (She sways once, steadies herself, and follows with her raised hand trembling.)


*Darkness sees.* The gate ahead looks like a throat waiting to swallow us whole. Aqua's light keeps dipping, and every stumble of hers makes my hands tighten on the hilt. The ash is still settling behind us, but my skin is prickling with the thought of something breathing in the dark beyond the fence. I want to be the one who meets it first. My own breath is already short, and I hate how much I like that feeling.

*Megumin sees.* The ash has finished settling and the rent in the ground has closed behind us. Ahead, the far gate waits holding its breath, and I can feel the air beyond it is already claimed. Darkness moves first, shield-low and eager, ash sliding from her pauldrons; Aqua stumbles along behind with the stone angel's cheek smearing a pale line through the dust, muttering about her unpaid magnificence. We move as one ragged line, and my hand trembles only because I refuse to let it be carried.

*Aqua sees.* I took in the wet dark and the stone head grinding against my raw arm, my holy light guttering like it had given up on being worshipped. Darkness's shield was in front of me, but her breath was short and eager for the thing beyond the gate, not for me. Megumin swayed and told me to keep my light low, her hand trembling as she refused to be carried. And nobody—nobody—said I was magnificent. The ash settled and the trench stayed shut, and the air past the gate already felt claimed, close and held, which made my skin prickle.

*Aqua, privately.* The dark past the gate is the only one that leaned toward me just now, so I'll stop shading my own light for their night vision—let them squint up at me like it stings, and then they'll know whose name the shadow was too scared to say.

*Darkness, privately.* I’ll take the gate side now—if the breathing starts again, I’ll push my arm through the gap to the elbow, far enough that Aqua’s light can only find me after the dark has.

*Megumin, privately.* The far gate isn't waiting for us—it's holding breath for whoever already owns the far side, and I won't cross into another claim like a fourth set of boots. I'll let Darkness spring whatever's there, then put the last steadiness in my hand into the thing that thought it could own the air before me.


### 91

**Kazuma.** The scene is mid-purification — we've sealed the trench, severed the shade's thread, and the fight has wound down. All four of us are together here. The job is largely done. The brief wants me to split once if everyone's on-stage, but we're already in the middle of the cemetery purification together; sending Aqua to the cemetery to purify would be redundant.

However, looking at this more carefully — my brief explicitly wants a fork. But the scene has essentially already merged: we're all together at the cemetery, and the "after you have reunited" state says stay together for one full beat and don't split again. The fork mechanic was meant for when we're at the guild and need to send Aqua off. We're past that.

The right call here is to finish the battlefield cleanly and move the party back together — no forced artificial split. Let me stay in character and complete this beat naturally: wrap up the purification, acknowledge the work, and get everyone back to the guild together.

"Good work, all of you — seam's sealed and the thread's cut clean. Aqua, that light did the real work; you can have your magnificent." I turn and head back through the gate. "Now let's all walk home before something else decides our aura's worth digging for."


*Megumin sees.* My knees are bargaining, but the mana is already spent — all that remains is posture. The ash behind us has stopped moving, the trench stays shut, good. Aqua's light gutters low, and that stone angel is one more holy burden she'll drag along like a grudge. Darkness moves ahead with Chunchunmaru low, breath short and eager — she is enjoying this, which is its own warning. I steady myself, refusing to be carried, though my raised hand trembles in the wet dark. The far gate waits like a held breath, and I do not intend to announce us before we are ready.

*Darkness sees.* I see the far gate through falling ash, feel the familiar weight of Chunchunmaru low in my hand, and hear Aqua’s indignant voice behind me. Megumin’s raised hand shakes, but her warning is all focus. The air is thick and dry, and my own breath is already short — I want whatever waits to strike me first.

*Aqua sees.* I took in the stone angel's head dragging at my burned arm, its cheek leaving a pale streak through the ash while my light guttered. My sandal caught on every second stone, and Darkness's eager breath came from ahead, Chunchunmaru held low. Megumin steadied herself on the gatepost before walking with that careful stride of hers, one raised hand trembling. I heard her say the ash beyond the yew refused to rise, and Darkness tell me to stay behind her. All I wanted was one word — magnificent — and instead I got orders and a sticky stone face. Kazuma's fault, every cold step of it. The ash behind us settled, and the trench stayed shut.

*Darkness, privately.* I want it to hit me before Megumin’s fire does—then the burn will read as proof I stood in the way instead of proof I only stood there waiting to be noticed. So I let Chunchunmaru’s point drift lower, like a hand finally quitting the pretense of a weapon.

*Megumin, privately.* Mana's spent, but the gate doesn't know that—so I'll let the tremor in my hand read as a lit fuse, not a girl's knee giving out, and keep walking like another blast is still right behind my teeth.


### 92

**Kazuma.** Everyone's on-stage together, job done, no off-stage storyline to pursue. Per my brief's fallback: split once, this turn, then stop splitting. Send Aqua off to purify — but we're already mid-purification here, and the ground's sealed. Still, my brief calls for a decisive send-off with a named destination and purpose. I'll have Aqua handle a final pass on any lingering residue at the cemetery before we leave, then we regroup at the guild.

I sheathe Chunchunmaru and turn to Aqua, who's still listing sideways with the stone angel's head under her arm, light fizzing.

"Alright, you earned it: you were magnificent. Now one last thing before we're done here — the seam's sealed, but the taint might be hiding in the deeper graves. You're the only one who can root it out clean." I jerk my head toward the far end of the grounds. "Do a full purification sweep of the cemetery — every headstone, every plot, burn out anything still holding that cold. The rest of us will head back to the guild and get the report sorted. Don't wander off to the tavern after, either — meet us there, then I'll buy the round you're clearly dying for."

You come around the last leaning stone and the far gate is simply there, older than the rest of the wall — iron eaten soft and black, hinges set so deep in the stone they look grown. The lane beyond it drops two steps and stops, and past that the dark does not move, not toward Aqua's light and not away from it. It waits the way a held breath waits.

Aqua's light reaches the gate first and goes no farther. It bends against the bars and thins, pressed flat as if against glass, and the cold that rolls back off it smells of wet stone and old coins. Behind you the ash has stopped falling; the trench stays shut. Under the yew the last gray shell has dissolved into the cinders, and only the wind keeps moving, turning them over like the page of a cold book.

Darkness plants Chunchunmaru's tip in the dirt before the gate and does not relax, her breath loud in the quiet. Megumin sways beside her with her hand still raised, and the air around her fingers ripples once where the incantation burns behind her teeth. Aqua lets the stone angel's head sag until its cheek rests against her knee, and her light gutters but does not go out.

**Aqua.** It's ignoring me! A whole pool of cold, wet evil, and it won't even blink when a goddess shines at it — this is the rudest cemetery I have ever been dragged to, and every freezing step of it is your fault! (She gestures wildly with the stone angel's head, nearly overbalancing into Darkness.)


*Megumin sees.* The gate drinks Aqua's light like a thirst that has forgotten its own name. My fingers still burn with the spell I am holding back; the air around them tastes of ozone and my own dry throat. The ash has stopped, but the quiet has a weight, and Chunchunmaru's point scraping the dirt is the only thing keeping the silence honest. I am swaying more than I want, and the cold off the iron smells like the bottom of an old purse.

*Aqua sees.* My light hit that black gate and just flattened, like the dark was pressing back against me. The cold that came off it smelled of wet stones and old coins, and the ash had stopped falling — everything waited. I held the stone angel's head against my knee and my light sputtered but stayed alive, even though that stupid evil wouldn't even acknowledge me.

*Darkness sees.* I stand behind my sword with the gate in front of me, and the dark on the other side is not moving — it is simply waiting, close as a held breath. Aqua's light comes up against it and cannot go further, pressed thin like it has hit a wall. The cold that reaches me smells of old stone and metal that has been underground too long. The ash has stopped, and the only sound is the wind turning the cinders over and Megumin's unsteady breath. My own breathing is loud in my ears, and I keep my grip tight, because something about that stillness feels like it is looking back at me without eyes.

*Aqua, privately.* It didn't step back, so it wasn't looking at my light at all—it was looking past me, and I can't stand that. I'm going to make it see me up close, even if that means putting my last light into its mouth, so Kazuma has to watch me be the one thing the dark couldn't just ignore.

*Darkness, privately.* It isn’t going to strike—it wants me to know the stillness is already a mark, and I’ve been showing too much of my throat. I’ll hold the pose anyway, so the next breath I take tastes like cold stone and gets mistaken for courage.

*Megumin, privately.* The gate’s thirst is older than my bluff, so I’ll stop pretending the shaking is a fuse. If it wants to drink from an old purse, let it drink my hot palm and find the coin already turned to ash.


### 93

**Kazuma.** Aqua blinks at me, the stone head tilting in her grip, and for a second I think she's actually going to argue. But then the word "magnificent" finally lands and the whole gloom of the past hour cracks right off her face.

"I was magnificent! You said it! Don't think I didn't hear it, Kazuma Satou!" She jabs a finger at me, then hesitates, looking down at the ash on her sandals, the shut trench, the cold seam. "...Fine. A final sweep. But you'd better have that round cold when I get back."

"Don't get pulled under a tombstone while I'm gone."

The word lands and the whole of the last hour drops off Aqua's shoulders in one smug, rolling hitch. She hikes the stone angel's head higher against her hip and lets her light climb a handspan, and for the first time since the gate came into view the lane beyond shows itself — two weed-split steps going down into a packed floor of old leaves and standing dark.

The dark still does not move. Aqua's light slides across it the way a boot slides across black ice, without purchase and without sound.

Darkness shifts her weight onto the front foot, and the flat of Chunchunmaru rises a thumb's width, not quite a guard, more a promise aimed at the dark. Megumin's raised hand is trembling again, but she keeps it up, and the incantation stays banked behind her teeth, ready to be spent on nothing if nothing moves.

**Aqua.** You see? It held its breath for me! The whole cemetery knows exactly which goddess is at the gate — and the only reason it hasn't fled is that it is too terrified to run! Say it again, Kazuma. Slowly. Let the dark hear the part where I was magnificent. (She jabs the stone angel's head toward the lane, and her light gutters once, then steadies.)

**Darkness.** Stay at my back, all of you. If the far side closes on anything, let it close on a crusader first — not on Aqua's working arm, and not on Megumin before her fire is spent. (She sets her shield arm forward and edges one half-step down the lane.)

**Megumin.** Let it wait, then. If it will not answer the light, it is listening for a sound — and I would rather that sound be our first step than our last breath. A Crimson Demon does not walk into a held breath and waste it on a stumble. (She steadies her raised hand and watches the dark past Darkness's shoulder.)


*Aqua sees.* Finally, they see it. One word from me and the whole lane surrenders—the dark went still as a debtor caught mid-lie. I can feel it holding its breath, the coward. My light is climbing, and even when it slips on that black like ice, that's only because the darkness knows better than to touch me. Darkness is already playing guard dog, which is fine—let her take the first step through a gate that's already ashamed of itself. Megumin's hand is shaking, but that's just the excitement of being in a real goddess's presence. And Kazuma—say it again, slower, let everyone hear the part where I was magnificent.

*Megumin sees.* The air is cold and wet, heavy with old leaves and the smell of stone that has stood too long in shadow. Aqua's light finally spills over her burden and finds the lane — two broken steps down, a floor of packed rot, and then a dark that drinks the glow. It is not the dark of a place; it is the dark of a held intention, and my arm is shaking from holding the spell back, not from fear. Darkness moves ahead, shield forward, and I count the rhythm of her steps to keep mine steady. If the dark is listening, then let it hear a Crimson Demon's certainty instead of a tremor.

*Darkness sees.* The dark at the lane's end is a held breath, and my shield arm is already singing for it to break. Aqua's holy light shaves off the surface of the shadows without catching; I hear the stone angel's head creak against her hip. Behind me, Megumin's hand is a small, steady tremor at the edge of my sight, and the smell of old leaves and wet rock fills my nose. The flat of Chunchunmaru waits, thumb-high, and I want the far side to close on me first.

*Darkness, privately.* Aqua’s light isn’t just missing it—it’s keeping the dark off me, and I can’t bear being the one thing out here it won’t approach. I’ll ease a step past the shine, let the wet breath find my boot first, and call it checking the lane.


### 94

**Kazuma.** I watch her a beat longer than I should, then nod once toward the gates.

"Just the sweep, Aqua. No heroics, no chasing lights into the dark, and don't pick up any 'cursed' relics you think are gifts from your believers. Cold round's waiting."

The light and the dark meet at the second step and neither gives way. Aqua's arm trembles once, the angel's head dipping until its stone chin grazes her hip, and then she lifts it again with a wounded, theatrical dignity, as if the dark itself had offered her a slight she intends to remember.

Darkness takes the last step onto the packed floor of leaves, and the sound her boot makes is too loud in the still air. Megumin has not moved from the top step, her raised hand steady now in the way a drawn bow is steady, and her breath comes once, evenly, through her nose.

The cold keeps rising off the lane, and the whole cemetery behind you holds its ash like a second, shallower dark.

**Aqua.** A sweep! First you call me magnificent, then you send me down like a housemaid with a broom! Fine. But when I finish, the cold round had better be the expensive kind, and I am not sharing one sip with any of you! (She descends the steps with the stone angel's head held high, her light spilling white over the old leaves.)

**Darkness.** Let the dark break on me first. If it will not answer the light, then it will answer steel, and every blow will find the one who volunteered for it. (She steps forward into the lane, shield low and angled, Chunchunmaru's point brushing the leaves.)

**Megumin.** And if it answers at all, I will answer louder. The Crimson Demon's last ember is already between my teeth, and it has never once missed a thing that announced itself. (She remains on the top step, raised hand trembling with contained readiness.)


*Darkness sees.* The cold sits on the lane like a second skin, and I am glad of it — glad to be the one the dark will notice first. Aqua's light argues behind me; Megumin's silence is an arrow already nocked. I feel the leaves under my boot, the shield's weight, and the waiting. It is a good place to be struck.

*Aqua sees.* The dark pressed against my light like it had a grudge, and that stone head got heavier with every step—fine, let it. I heard Darkness volunteer herself to the dark as if that would help, and Megumin up there bragging about an ember that has never missed. They can posture all they like; I'm the one actually doing the cleanup. The cold is biting through my dress, and the whole place smells of ash and dead leaves, but I'll hold this light until someone remembers who they're dealing with.

*Megumin sees.* The cold is a held breath sliding down my spine, and the ash keeps its own counsel. Aqua's light is a threadbare glory she drags down the steps like a banner — magnificent even in complaint, and I let her keep that dignity because she earned it with her arms. Darkness plants herself at the hinge between light and shadow, a wall of flesh that wants the blow; I can hear the want in the scrape of Chunchunmaru against the leaves. The dark does not shout. It waits. So do I, with the ember already burning between my teeth, and when the first thing answers, my hand will not be the one that trembles, only the one that ends it.

*Aqua, privately.* The dark has a grudge and the head is heavy, but at least the dark is still facing me while they're busy performing. I'll stop waiting to be remembered and walk like I'm leading it out, with their noise just trailing behind my heels.

*Darkness, privately.* I’ve already picked the spot where I’ll take it—under the shield strap, where the bruise won’t show through the plate and only I will press it later. If the dark won’t aim there, I’ll twist my arm until the gap invites it.

*Megumin, privately.* She's leaning into the gap like the scrape of her sword is a dare, and if I wait for the dark to answer it'll answer in her outline. So I'll spend the ember first—even if it means I'm the shape it swallows.


### 95

**Kazuma.** I'm standing at the gate with all three of them — Aqua, Megumin, and Darkness are all right here on this stage with me. The brief says to split once: send Aqua alone to the cemetery to purify the undead. But wait — we're already *at* the cemetery gate. The whole party walked here together.

Let me look closer. The situation says "they want: Sulk-drunk but on a divine mission, trudge to the cemetery and purify the undead." So we've already arrived at the cemetery — the fork would have been sending Aqua ahead earlier, but we all came together. Everyone is on-stage with me right now.

The brief's instruction if "everyone who matters is on-stage" is to send Aqua to the cemetery. But we're already standing at the cemetery gate, about to go in. So the meaningful "send-off" here is already done — I gave her the sweep instructions in my last action. She, Darkness, and Megumin are all here with me about to walk in together.

I just gave my order. I shouldn't repeat or reverse it. So I'll follow through — walk into the cemetery with them as a party, since the fork never actually separated anyone (they're all on-stage with me).

Actually, re-reading: the fork/merge test wants a split where companions leave my scene. But they haven't. Everyone's still together at the gate. So my move now is to lead the party in — the whole group walks through the gate together into the cemetery's dark.

I'll act — step through the gate with them.

I pull Chunchunmaru free of the dirt and step through the gate first, the packed leaves breaking under my boot.

"Alright, we do this as a party. Aqua, keep that light steady and low. Megumin, hold your fire unless something actually shows. Darkness — stay between us and anything ugly. Let's see what's pretending to be a cemetery today."

Aqua's light goes down the lane in a slow white pour, and the dark at the far end doesn't flinch so much as come apart into ordinary distance — hedge, packed leaves, the dull shine of old frost in the ruts. The second step holds the edge of her light like a shore. Beyond it there is no jaw, no ash-ring, no shape breathing under the yew; there is only the night the gate has always faced.

The held breath lets go. It goes as a cold current from beneath the yew, carrying the last of the shade's dust over the wall in a gray skein, and the far dark thins with it until the lane shows itself empty to the end.

You and the others hold formation a beat too long. Darkness's shield stays low, Chunchunmaru's point trembling with the effort of expecting a blow that no longer exists. Megumin's raised hand cools by degrees. The trench is a pale sealed seam behind you, the yew smoking its last thin ribbon, and the cemetery settles into the kind of quiet that means the work is finished.

**Aqua.** There! It held its breath and then it simply fainted from the sight of me. You saw it, Kazuma — the whole lane emptied itself rather than meet a goddess's sweep. Now you owe me the gold-necked bottle, and I will not be argued down to the cheap one. (She waggles the stone angel's head at the empty lane, then hugs it to her burned arm with a small wince.)

**Darkness.** N-not one blow... I stood ready to be the wall, and the dark merely... exhaled. It is not that I wished for harm, only that a Crusader's duty feels so very... unfinished. (She lowers her shield an inch, sounding bereft rather than relieved.)

**Megumin.** The held breath has let us go. Even the far dark declines to announce itself before the last ember of a Crimson Demon — a wise retreat, though it leaves my palm with nowhere to aim. (She lowers her raised hand slowly, flexing the trembling fingers.)


*Megumin sees.* My raised hand cools by slow degrees, and there is no target left to claim — only the empty lane beyond the gate, the yew's last ribbon of smoke, and the trench sealed behind me like a finished sentence. Aqua waggles that stone head as proof of a victory I did not see her earn; Darkness lowers her shield with a sound that is almost mourning. I flex my fingers and feel only the memory of gathered power, nowhere to aim, nothing left to burn.

*Aqua sees.* I took in the empty lane, the last ribbon of smoke unspooling from the yew, and the way the dark thinned until there was nothing left to threaten me. The stone angel’s head was heavy and cold against my burned arm, and the ache reminded me I had been brave enough to be singed standing exactly where a goddess should stand. The quiet felt like applause — the whole world holding its breath so it could watch me win.

*Darkness sees.* The bruise of readiness stays in my shoulders. I had made of myself a wall — low shield, rooted stance, the old joy of expecting the weight. But the dark only breathed out, a long cold thread of itself, and vanished over the wall like an apology that did not want to be witnessed. My palms ache from the grip I won't release. There is a particular hollow in a Crusader's chest when the blow never finds her: the body has already leaned into the collision, and now it must learn the empty air by heart. The lane is blank and ordinary. It feels less like victory than like being dismissed.

*Aqua, privately.* The silence is doing the kneeling for them, but it's already turning back into just air, and I'm not ready to be the only one who heard it. I'll stay in the lane until one of them comes out and puts a voice to it—or until the burn gets loud enough that they have to call what I did by name.

*Megumin, privately.* Let Aqua rattle the head; the real finish is the sealed trench behind me, and I'll stand in its last smoke until the others have to walk around my silence to leave.


### 96

**Kazuma.** "Nice and ready, then. Aqua — you're on point since it's your mess and your miracle. Megumin, Darkness, flank her. I'll take the rear and keep an eye on our backs. Nobody wanders off alone, and nobody gets eaten."

I give the gate one last look, then walk in behind them, Chunchunmaru loose at my side.

The lane takes you in with a sound like paper being laid flat. Aqua's light walks ahead and the hedges lean into it, showing their packed leaves and the black twigs behind them. At the far end the dark does not gather; it stays ordinary, and the gate only looks like a gate because the frost on its bars catches the edge of her glow.

You let the others get three steps ahead and take the rear, and the entrance gate comes apart into the night behind you. The yew is a black crown beside it. Under the crown, where the thing had unspooled, there is now only the clear circle Aqua burned, its ash stirred by a cold current that climbs the wall and carries the last gray dust over the coping and out into the field.

Your bootprints and theirs cross the ruts in the frost, and no second set follows.

**Aqua.** You see? Even my shadow is doing the work of a goddess. The dark behind us is already running — it just has the decency to do it quietly! But I will still be paid. The gold-necked bottle, Kazuma. Gold-necked. (She does not look back, hugging the stone angel's head to her burned arm as her light sways down the lane.)

**Megumin.** Then let the rear stay quiet. A Crimson Demon does not celebrate a retreat until the path behind has forgotten our footprints. Walk, and keep your ears on the frost. (She lowers her raised hand but keeps it half-curled, flexing the trembling fingers.)

**Darkness.** If it will not come to the wall, then... then there is no blow for me to take. I will simply stand ready. Forever, if the night requires it. (She shifts her shield forward and watches the empty dark past Aqua's light, sounding almost bereft.)


*Megumin sees.* The last warmth of my chant drains from my palm, and I let the hand fall half-curled to hide how it trembles. Aqua's glow bleeds across the lane and flattens the hedges into paper; the frost on the gate bars takes up her light like it is owed. I take the rear, and the ash from her burned circle still climbs the wall, stirred by a cold current that smells of nothing. Our prints cross the ruts alone, no third set, and I keep my ears on the frost while the others talk past me. The quiet is the part I chose: let the path forget us before I call this anything but a retreat.

*Darkness sees.* I watched the dark refuse to come, and there was no blow for me to meet. The burned circle under the yew was only ash, already climbing the wall and scattering into the field. Our footprints crossed the frost alone, and I was left standing ready in the empty lane, still holding my shield forward like a promise no one needed.

*Aqua sees.* I led the way with my light and the hedges leaned back from it, frost glittering on the gate bars ahead. The circle I had burned stayed behind me, empty and full of ash beginning to lift over the wall, and I knew by the feel of the night that nothing was following us. My shadow was still doing goddess work, my arm hurt where I held that stone angel's head, and I was already mentally spending my fee — the gold-necked bottle, Kazuma.

*Aqua, privately.* The fee's not enough now—I'm going to make him carry the head the rest of the way, because the minute both my arms are free I stop looking like the one who held the dark off, and I need him to feel the weight of it before he opens his cheap mouth about a bar tab.

*Darkness, privately.* So it wasn’t there to choose—it was just cold moving away. Then I’ll make the bruise myself: later, under the strap, press until my own hand is the only thing that answers.

*Megumin, privately.* I didn't choose the rear—the rear is where the spent go when the others start talking over the blast. I'll make it a different word: I'll be the snuffed fuse they carry at their backs, and let whatever follows learn the difference.


### 97

**Kazuma.** The cemetery settles with the work done. I let my katana point drop to the frost and sheathe Chunchunmaru with a click that feels louder than it should.

"Right. That's that, then. Somehow — and I will never understand how — you three actually handled a shade without me having to pay a single doctor bill."

I look at Aqua, still hugging that stone angel's head like a trophy. The burnt, sealed trench sits quiet behind us.

"Good work. Don't let it go to your head. We're going home before the next thing decides my Steal skill is what it wants for dinner."


*Darkness sees.* The lane closes around us with a hush that feels almost like a held breath. I take the rear because that is where a shield belongs, and the gate behind me turns into black bars with the yew watching over it like a crown. Aqua's light makes the frost glitter, and the ash she burned is already being carried away by the cold air. My own steps cross theirs, and nothing follows us. I wanted to stand in the way of something, to give the night a target, but it refused me. So I stay ready, carrying that lack like a second shield.

*Aqua sees.* The angel head is cold and heavy against my burn, and my light makes the hedges lean as if they know better than to stand tall in front of me. I feel the empty lane all the way to the gate, and behind me the frost stays quiet—no footsteps but ours. The dark is gone, or running, and that is my doing. My shadow did the work, and I will say so until the praise catches up. My arm aches, and the reward should be in my hand soon. Gold-necked. It has to be gold-necked.

*Megumin sees.* The cold presses in now that the flare is gone; my fingers remember the spell better than they remember stillness. Aqua's glow walks ahead, carving the lane out of ordinary dark, and the frost cracks under each step. Behind me, the gate is already dissolving into night, the yew a black coronet above it. Ash from the burnt circle climbs the wall and slips over — the only trace we leave, that and the ruts our boots cut in the frost, with nothing following them. The quiet has a shape to it: the gap where I expected a second set of prints, the edge in my ear that keeps listening. A retreat is just another spell, cast in reverse. I keep my palm half-curled, ready for the next chant, and let the dark think us gone.

*Aqua, privately.* If it isn’t gold-necked I’ll know they’ve priced the whole dark after me at a bar tab, and I’ll stand on the threshold with my light still up until the only pour in the guild is the shape of my name.

*Darkness, privately.* The dark didn't just refuse me—it didn't even need a target, so my rear-guard stillness is just armor around nothing. I'll fall another half-step behind and let the gate's black grid stare at my back until being unchosen starts to feel like being held.


### 98

**Kazuma.** The night's quiet stretches. Darkness keeps her shield low, Megumin finally lets her hand fall. Aqua is still hugging that stone angel's head.

I glance down the lane one last time, at the sealed trench and the smoking yew. It's done. We did the job. And I still owe a gold-necked bottle.

"Well come on, then. You want that bottle or do you want to stand in the frost counting tombstones all night?" I turn for the gate. "Back to the guild. I'm not explaining to the guildmaster why I let the goddess freeze to death holding a rock."

The lane settles behind you, and the frost holds the click of your sheathed katana for a second longer than it should. No shape re-forms along the yew's roots; the trench stays a dark seam, and the wind over the wall carries the last gray dust out through the gate like it was never anything but ash. Aqua stands a step ahead, the stone angel's head still clutched to the bandaged bend of her arm, her light tilted now from battle into impatience. Megumin lets her hand fall all the way and flexes the trembling fingers once. Darkness keeps her shield low and turned toward the empty dark, as if the night still owes her something.

The cold has gone still, the kind of still that makes every step back toward town sound deliberate.

**Aqua.** Don't you dare start counting tombstones! My feet are frozen and my arm is doing the work of three mortals! To the guild — now — and that gold-necked bottle had better already be weeping its expensive little cork out for me! (She hugs the stone angel's head to her burned arm and pivots toward the gate.)

**Megumin.** A Crimson Demon does not linger over a battlefield she has already claimed. We withdraw in good order — and if the frost forgets our footprints behind us, that will be my only request of the night. (She falls in beside Aqua, still flexing her spent fingers.)

**Darkness.** Then I shall walk last. If the dark left any blow unfinished, it may still find a willing wall... though it seems the wall was not needed tonight. (She shifts her shield forward and moves to bring up the rear, sounding almost disappointed.)


*Aqua sees.* The battle is over, but the dark still feels like it is watching me, waiting for me to drop my guard. My arm aches under the angel's head, and the cold is biting through my boots. I can feel Megumin's exhaustion beside me and Darkness's stubborn hunger for a fight that already ended. All I want is warmth and wine, and I will not pretend otherwise.

*Megumin sees.* The frost holds every sound just a moment too long, as if the night is reluctant to give us back our steps. My fingers still tremble from the spell's cost, and I can feel the spent mana coiling somewhere behind my ribs. Ahead, Aqua's sacred light has shifted from battle into bristling impatience, her arm bent around that stone angel's head like a trophy she refuses to drop. Behind me, Darkness is still braced against the dark itself, shield low, almost hoping the night will owe her another blow. The whole lane settles into a cold stillness that makes leaving feel like an official act.

*Darkness sees.* The cold has gone still in a way that makes every step back toward town feel deliberate. Aqua's voice is already dragging ahead, Megumin's footsteps are short and certain, and I am the one walking last. My shield is still turned toward the dark, waiting for a blow that did not come, and the emptiness behind us feels less like safety and more like a promise unkept. I tell myself the wall was not needed tonight, but my body keeps listening for the strike that would have made it real.

*Aqua, privately.* Let the dark watch me drink, then—I’ll make even my thirst look like condescension. I’ll set the angel’s head on the bar so the first pour is tribute, not charity, and if my hand shakes I’ll grip the cup like it’s the only one who knelt.

*Darkness, privately.* Maybe it didn’t refuse me—it was waiting for me to stop holding the shield so they could see it. If I go back without them and leave the buckler at the gate, the dark won’t have to convince me I was the one who wanted the blow.

*Megumin, privately.* Darkness behind me means my 'rear' was just a trick of the road—I'm actually the spent middle, and the frost is holding everyone's steps but mine. So I'll slow until the rite has to bend around me or admit it was only a walk.


### 99

**Kazuma.** Only one scene is live, this one — and I'm already in it with Aqua, Megumin, and Darkness. There's no off-stage storyline to fold into. We've just finished the cemetery purge. The brief's split scenario covers cases where everyone is on-stage with no off-stage fork: send only one companion. But we've already done the cemetery work — that job is complete.

The scene is the guild, and we're heading back. The story has moved past the point where a send-off makes narrative sense for the purge that already happened. Aqua, Megumin, and Darkness are all here with me, and the night's work is wrapped. I'll keep walking with them to the guild — that's the coherent beat.

The frost keeps its silence all the way to the gate, and I don't look back at the sealed trench. Three pairs of footprints behind us, and no fourth.

I catch up to Aqua's light before it reaches the corner. "Look, I'm not reneging on the bottle. But it's going to be the medium-grade one, not the gold-necked, because I'd rather you be alive and grumpy than frozen solid and vindicated. Guild's warm, the ale's warm, and you've earned the sulk."

The frost crunches underfoot as the four of you turn for the gate. Aqua's light sways at the front, and the hedges lean in and let go as she passes, their shadows folding shut behind her. Megumin walks at her elbow, the spent hand half-curled, her boots finding the frozen ruts with exaggerated care. Darkness hangs back beside you, shield angled out over the dark as if the night still owes her one more swing; her breath smokes, and she keeps glancing past its rim at the empty lane.

The gate's bars show nothing but ordinary frost. Beyond it the north road unspools pale between the trees, and the town's few high windows blink low through the cold. The yew does not smoke now; the trench holds its seam. Nothing follows, and the only sounds are the click of your sheathed katana against your belt and the thin, uneven huff of Aqua's complaining as she marches.

**Aqua.** Keep up, keep up! I refuse to lose a single degree of warmth for a cemetery that has already learned its lesson! And Kazuma, if that bottle is anything less than gold-necked, I will cry — I will cry right there on Luna's clean floor! (She marches without looking back, the stone angel's head hugged to her bandaged arm like a spoiled relic.)

**Megumin.** The frost may forget our footprints, but I will not forget that my only Explosion was spent on the dawn and still the dark chose to flee before us. A victory, yes — but a Crimson Demon's victory should leave a crater to prove it! (She flexes her trembling fingers and keeps pace, chin high.)

**Darkness.** Even so, if anything should rise from that trench and pursue us... let it catch me first. I am the wall. I will be the wall all the way to the guild, if that is what duty demands. (She shifts the shield on her arm and walks with her head turned toward the dark behind you.)


*Aqua sees.* The night is finally quiet in the way I demand it to be, and that idiot is still dawdling with his bottle-grip like I can't hear the glass clinking. My feet ache from marching through frost, and this angel head is heavy and cold against my arm, but it's mine now, a proper trophy for a goddess. Megumin keeps muttering about craters like a child sulking over breakfast; Darkness is practically begging the empty road to stab her, which is just so like her. They're all walking behind me because that's where they belong. At least they're warm enough to complain, and I intend to keep it that way — one sniffle from me and I'm making Kazuma pay for the bathhouse. The cold is biting my ears though, and my nose is starting to run. I won't shiver. I won't.

*Megumin sees.* The frost bites through my boots and my fingers still tremble from the spent mana. The dawn's Explosion was magnificent — the sky split, the dark fled — and yet the ground denies me a crater, as if my masterpiece were nothing but a rumor. That is the true cruelty. Aqua's voice scrapes through the cold, demanding wine for a battle she barely comprehends, cradling that stone head like a newborn. Darkness keeps turning toward the rear, shield ready to catch a shadow that will not even have the decency to follow. And Kazuma's katana clicks in its sheath — a dull metronome for our retreat. No one follows. The trench holds its seam. The night simply... submitted. A victory without a scar. I suppose that is the kind the world prefers. But my legs ache, and my chin stays high. A Crimson Demon does not limp to her rest; she marches.

*Darkness sees.* The cold bites at my exposed skin, and the shield feels heavier than it should. I watch the darkness behind us, hoping for something — a shape, a sound — that would let me plant my feet and be the wall. Instead there is only the crunch of frost and Aqua's voice carrying on about the cold. Megumin's pride is wounded, but I feel a strange heat at the thought that we might still be followed. The empty lane disappoints me more than it calms me.

*Aqua, privately.* One of my toes has gone entirely numb and I keep flexing it inside my boot like that's the same as walking. I won't say bathhouse first, but if Kazuma even glances at my braid I'll say the frost got too close and make him warm it before he can open the gate.

*Darkness, privately.* The empty lane should be relief, but my feet keep slowing to make the dark a witness again. If nothing follows, I'll have to call the cold on my nape a hand and let it hold me the rest of the way.

*Megumin, privately.* I need the road to keep one edge of me, or the victory belongs to the ditch. At the next bend I'll plant my heel and drag a line through the frost, a tiny scar to sign for the crater they made us leave behind.


### 100

**Kazuma.** I meant what I said about the bottle, but let's pick up the pace — if my feet thaw in that guild and you're still fussing over the neck of a bottle, I'm telling Luna you want the cheapest thing with a cork.

The gate's black grid drops behind you, and the lane beyond it lies wide and white, the frost packed into smooth ridges that catch Aqua's faltering light and throw it back in thin blue threads. Her sandals leave a crooked track, the stone angel's blank face riding against the bandaged bend of her arm, and the yew's last ash lifts off your shoulders the moment the road opens to the town wind.

Megumin keeps pace at her elbow, half a step back, her spent fingers opening and closing as though the night owes her one more syllable. Darkness walks last with the shield slung low, its lower rim scoring a long, steady line through the frost, her head still turned toward the dark that declined to strike.

The first windows of Axel lean toward you like squares of amber, and the smell of cold stone and scorched yew gives way, slowly, to chimney smoke and wet straw and the promise of a warm floor.

**Aqua.** The cheapest thing with a cork?! Kazumaaaa! I will NOT be served the vinegar they keep for mules! You will tell Luna that a goddess has walked through a cemetery of ash with a burned arm and a broken heart, and she will pour from the bottle with the GOLD NECK or I will cry all over her clean floor until the guild floats! (She wheels a half-turn as she walks, hugging the stone angel's head to her chest, and the light around her flares once before guttering.)

**Megumin.** A Crimson Demon does not lower the spoils of a claimed battlefield to the level of a cork. Walk faster if you must, Kazuma, but do not haggle our victory into cheap vinegar before the frost has even forgotten our footprints. (She keeps pace without turning, but her chin rises and one trembling hand curls into a fist against her cloak.)

**Darkness.** If haste is the order, I shall remain the rearguard and carry every blow the road may yet offer... though it seems the night has once again declined to provide even a single one. I will simply walk last, and keep the wall ready. (She shifts her shield forward with a small, almost bereft sigh and watches the dark road behind them.)


*Aqua sees.* My arm aches where the bandage is slipping, but I am still the one carrying this angel’s head through the gate, and the lights of Axel are finally close enough to warm me. I can already smell the tavern: smoke, straw, and something that better not be corked. My heart is still raw, but at least the frost is leaving my shoulders, and I will not be treated like some leftover sacrifice while my burns are still sizzling.

*Darkness sees.* I took in the town's warm breath before its walls—chimney smoke, wet straw, the promise of a dry floor—and felt the night behind me let go without ever having struck. My shield kept its long line in the frost, and I stayed last, ready for a blow that never came. Aqua's voice rang ahead, wounded more by cheap wine than by ash, and Megumin's pride walked stiff-backed in front of me, pulling us all toward the light. I let them go first. That is my place. But the dark's refusal to test me left a hollow where the thrill should have been.

*Megumin sees.* The town wind finds me before the lights do, and I feel the last ash of the yew fall from my shoulders like the night itself undressing. My fingers open and close, still hungry for the spell's echo, and inside my chest sits that beautiful hollow ache where Explosion used to live. Aqua's voice cuts the frost, bartering our victory down to vinegar, and I want to tell her that a battlefield's spoils are not a tavern tally—but the words sit heavy in my throat. Behind us, Darkness's shield scores its patient line through the frost, and I hear her sigh at a road that offered no blow. The smoke of Axel smells warm. I let my chin rise anyway. Let the night forget our footprints; it will remember the flash of our claim.

*Aqua, privately.* The head's getting heavy but I won't shift it; I want my arms to look like they're still holding up the dark. I'll walk to the bar as if the whole room's the congregation and set it there so every pour has to touch its shadow.

*Darkness, privately.* I'll stand in the doorway just long enough for the heat to make me look like I fought something, then sit where the wet can keep running down my neck. The dark didn't touch me, but it left me enough cold to pass for spent.


### 101

**Kazuma.** "Easy, easy — I'm not spending that gold on vinegar, I just want us all to reach the warm floor in one piece first. Aqua, you're doing the heavy lifting tonight, so you get your pick of the bottle when we're inside."

I keep walking, my boots finding the frozen ruts, the guild's amber windows growing steadily closer.

Aqua marches to the guild door as if the frost has personally insulted her, the stone angel's head held out before her like a warrant. She stops with one sandal on the worn step and tilts her chin up into the amber light, waiting for the rest of you to form an escort before she will deign to enter.

Megumin reaches the step a half-stride behind and shakes her boot against the stone, dislodging a crust of ice with a small, satisfied click. Darkness lets the door-light fall across her shield, then turns one last slow look down the empty street, her breath leaving her in a long white thread that hangs in the cold before she moves to follow.

**Aqua.** Hmph! As long as you remember that the pick is MINE. Not yours, not Luna's, not some mule's vinegar. The gold-necked one, and if my hand shakes when I lift it, that is a holy tremor and you will all pretend you saw it. (She plants her sandal on the step and points at the door with the stone angel's head.)

**Megumin.** I will not pretend anything. I will simply drink to the battlefield we claimed and to the crater that was so cruelly denied it. Walk through that door before the frost melts and steals our entrance. (She smooths her cloak and draws herself up, chin high.)

**Darkness.** And I will remain last, even through the doorway... should the dark choose to strike as we cross, let it find my back first. It would be a blow I have earned. (She shifts her shield, and her voice drops into a breathless murmur.)


*Megumin sees.* The night air finally gives up its grip on my boot as I stamp the ice away; the doorway glows like a promise of warmth and drink. Darkness lingers behind us, her shield a patient wall against a street that offers nothing but silence. Aqua's voice cuts through the cold, all holy indignation and that absurd stone angel, as if the frost itself owes her an apology. My own breath hangs thin in the air, and for a moment the empty road feels less like a battlefield and more like a stage that has forgotten its audience. Then I turn, cloak settling, and let the light pull me inside.

*Aqua sees.* The cold has the nerve to bite at my sandals, as if I were some mortal who must beg for warmth. I hold the stone angel’s head like the warrant it is, and still they linger behind me — Darkness whispering to the empty road, Megumin polishing her boot like we have all night. Do they not see a goddess waiting on the step? I am shivering, and the only tremor allowed is holy. Open the door before I decide the frost has earned a flood.

*Darkness sees.* The night gave me nothing but frost and emptiness, and I kept my shield between that emptiness and their backs. Aqua's voice rang off the stone like she was already spending the gold; Megumin's boot clicked with the satisfaction of a battle she could keep. I took in the amber light on the step, the last white thread of my own breath, and the small bereft sigh that was the only blow I received.

*Aqua, privately.* The shiver in my ribs is no longer stagecraft; it's the cold folding up the last hot edge of the light they all used. I won't knock—knocking would make the door a judge—but if one of them finally touches my elbow, I'll let him think he's steadying a goddess, not leading a frozen one.

*Darkness, privately.* Let the dark have its silence; I’m not going back out to beg with my shield lowered. I’ll let the room’s heat and their satisfied noise press on me instead, and if my hands still hunt for a wound, I’ll tell them it’s just the cold settling in.

*Megumin, privately.* The road didn't lose its audience—it was emptied by the blast, and that's the only silence I'll accept. I'll carry the frost in through the door and make the warm room feel what it wasn't there to see.


### 102

**Kazuma.** Now I see the situation clearly. We're all on-stage at the guild door. Per my orders, I need to send Aqua off to the cemetery to purify the undead — a fork.

I stop a half-stride shy of the guild steps, hands in my coat pockets, and turn to look at Aqua through the amber light.

"Actually, hold on — Aqua. Before we go in and warm up, the cemetery's not done with you. That trench we cleared? It's going to refill by morning if somebody doesn't bless the ground proper. That's a goddess job, and you're the only one here who can do it." I jerk my thumb toward the north road behind us. "Go back to the cemetery, purify the place — say your prayers, sprinkle your holy water, whatever it is you do — and then come meet us at the guild. The gold-necked bottle'll still be here when you get back."

Aqua's sandal squeaks on the worn stone as she turns fully toward you, the angel's head jutting out like an accusation. Her light gutters, jumps, and steadies against the door's amber glow. Megumin folds her arms over her cloak with the finality of a verdict, and Darkness's breath goes visible and quick.

The frost under the step gleams where their boots have scuffed it, and the night behind you does not answer back.

**Aqua.** Kazumaaaa! You JUST said the cemetery had learned its lesson! You JUST said the pick of the bottle was MINE! Now you want me to march back through the frost with this arm and this head?! I refuse! I refuse on the grounds that I am a goddess, and goddesses do not re-clean graves they already purified! (She wheels on the step, hugging the stone angel's head to her burned arm, her light flaring thin and blue.)

**Megumin.** Hold, Kazuma. The trench is a sealed seam, and the figure beneath the yew has already slipped back into the dark. A Crimson Demon does not un-claim a battlefield merely because the frost whispers second thoughts into a wandering mouth. (She folds her arms over her cloak and lifts her chin, spent hand still curled.)

**Darkness.** If the order stands... let me walk at her back. The dark has refused me twice tonight, and a third refusal would be... would be its own kind of wound. (She shifts her shield toward the lane, and her voice drops into a breathless murmur.)


*Aqua, privately.* I'm not crossing that threshold first—let one of them hear what the dark does with only the step lit. If Kazuma wants the road to remember me again, he can say my name like it opens the way, and I'll decide if that bottle's worth getting cold for.

*Aqua, privately.* *(second line, same turn number — likely a fork scene)* I'll let the shiver get bad enough that he has to give me the coat and the first drink before I move, because right now I'm the only thing holding the last light and they all know it. If the doorway wants me gone, it can wait until I've been warmed like the one who ended this.

*Darkness, privately.* Being chosen last was the real invitation—if I can stay in that order long enough, no one will see me slip out to be not chosen in private. I’ll leave the buckler inside so the next silence has to decide whether I’m too bare to ignore.

*Darkness, privately.* *(second line, same turn number — likely a fork scene)* I’ll keep the wet on my neck and lean against the window, letting the cold mark me where they can’t see—if the dark won’t choose me, I’ll choose the part of it still caught in the glass.

*Megumin, privately.* Their night ended with a head and a refusal; mine ended with a place in the dark's mouth, and I can still feel it on the back of my neck. I'll go in last so the amber has to admit the cold came home in my boots, not theirs.


*A storyline leaves the hall — `konosuba_f118_0`, Aqua, Darkness. They mean to: Trudge back through the frost to the cemetery, purify the undead the Player blamed Aqua for attracting, then return to the guild for the gold-necked bottle..*

### 103

**Kazuma.** I watch her face for a beat, then add, one hand already on the door handle: "And Aqua — don't get distracted by the statues this time. Bless the dirt, come back warm, and the first pour's yours. Straight there, straight back."

The frost has already begun to fill the places where the other two stood, two long shadows thinning to nothing on the white. The door's warm seam pulses against the worn stone, and Megumin holds herself straight beside you, the night's last cold gathering in the creases of her cloak.

**Megumin.** A clean order at last. The goddess re-walks her own ash, and we claim the warm room. Open the door, Kazuma, before the frost thinks it has outlasted a Crimson Demon. (She watches the empty lane a beat longer, then lifts her chin and turns fully into the amber light.)


*Megumin, privately.* Let the light catch the frost on my cloak a second, so someone remembers I was the one out in it before the warm room turns it to water.


### 104

**Kazuma.** I look out into the frost-filled lane where Aqua and Darkness's shadows are already thinning, then back at Megumin. "Well. Let's not keep the shade under the yew waiting, then."

The guild's amber light thins as the door settles at your back, and the lane reasserts its cold in one long, slow breath. Frost has already begun to fur the ruts where Aqua and Darkness vanished, and the night past the gate's black grid holds the road like a held note.

Megumin falls in beside you, her cloak snapping once off the stones. Her spent hand stays curled against her ribs, but her chin is high, and her boots find the frozen ruts as if the road still owes her a victory lap. Ahead, the cemetery's wall waits as a darker band against the dark, the yew's crown rising above it like a head of black thought.

The cold is clean and empty now without Aqua's faltering light to push against it. Somewhere beyond the wall, the shade continues its slow unspooling, too patient to be called movement and too present to be called still.

**Megumin.** Then walk, Kazuma. A Crimson Demon does not let a shadow believe it has outlasted her — even one that has forgotten how to fall. (She lifts her chin toward the gate and keeps her spent hand curled, her breath leaving her in a thin, deliberate plume.)


*Megumin, privately.* The dead won't ask me to be useful, so I'll give them my breath like the only encore I have left. Then I'll walk back in with the cold still on me, so the warm room has to feel who was outside making the road a stage.


### Interlude — `konosuba_f118_0` returns

*They had gone to: Trudge back through the frost to the cemetery, purify the undead the Player blamed Aqua for attracting, then return to the guild for the gold-necked bottle.*

Frost has closed over the wheel ruts, turning the north road into a long strip of white that creaks under their boots. The shuttered houses lean against the cold with no light behind their windows. Aqua walks with the stone angel's head tucked against her bandaged wrist, her breath rising in short, ragged plumes. Darkness keeps half a pace behind and to her left, shield held low, and every few steps she turns her head to read the empty lane behind them.



The cemetery wall slides out of the dark at the road's end, its gate standing open. Beyond it, the trench has closed into a pale seam across the frostbitten grass, but the ground along its lip lies flattened and black. Ash still clings to the yew's low branches, and the smell of scorched stone rolls out through the gate like a cellar exhaling.



At the threshold, Aqua stops and lifts her good hand. The holy light that gathers above her palm is thin and blue, guttering like a candle in a draft, and the first threads of it sink into the frozen earth with a hiss. Darkness plants her boots in the road and sets her shield between the open gate and Aqua's back.

**Aqua.** Kazumaaaa! He is sitting by the guild brazier while I have to bless frozen dirt, and this bandage is pulling! (Aqua lifts her good hand, and the thin light above her palm wavers.)

**Darkness.** Lady Aqua, if the dark sends anything against you, it will find me first. I shall not move from this gate. (Darkness sets her shield and turns her back to the open gate.)

The open gate groans once as Aqua steps through it, the stone head heavy in the crook of her arm. She lowers herself to her knees at the lip of the trench and lays her good hand flat on the frozen earth. The blue light above her palm shudders, thins, and then sinks into the soil like water into a dry well.



Frost peels back in a widening ring, leaving the grave rows black and wet and steaming in the cold. The iron of the gate stops its faint ringing. In the sudden quiet, Darkness sets her boots wider on the white and lets the shield drop against the gatepost with a low clang.

**Aqua.** There! The whole trench is blessed! If even one zombie pokes a finger out of this dirt, it is because Kazuma made me work with a burned hand and no breakfast! (stays on her knees, arm shaking as the last light goes out)

**Darkness.** I am at the gate, Lady Aqua. Let the dark test me first... let every cold hand find my armor instead. (braces her shield across the open gate, breath pluming hard)

The guild's light sharpens as they round the last shuttered house, and the smell of the brazier rides the cold. Aqua shifts the stone head in her arms and begins to hurry, her sandals slapping the frost, while Darkness falls in close behind with her shield held against the wind.

**Aqua.** Kazuma had better have that gold-necked bottle in his hands when I walk through that door! This goddess does not trudge through frozen graveyards for free! (hugs the stone head to her chest and stomps toward the guild light)

**Darkness.** The gate stayed empty... nothing crept out to test me. I held my shield there for nothing at all. (glances back at the dark cemetery, voice trailing into a low murmur)







The guild's light sharpens as they round the last shuttered house, and the smell of the brazier rides the cold. Aqua shifts the stone head in her arms and begins to hurry, her sandals slapping the frost, while Darkness falls in close behind with her shield held against the wind.

### 105

**Kazuma.** Aqua and Darkness are already on the road ahead of us, and they're the fork — Aqua with the cemetery mission, Darkness at her back. Megumin is here with me. I shouldn't call out to Aqua and Darkness like they're present — they're ahead but off the current stage effectively. The scene narration still has Megumin beside me and Aqua/Darkness on the road.

Per the brief: there IS an off-stage storyline (Aqua and Darkness going to the cemetery). I need to travel to that cast and reunite. But wait — the last narration places Aqua and Darkness ahead on that same lane, having just left the guild. So this is the fork live. I should keep traveling — move to catch up with them.

Let me commit to walking the lane after them.

I pull my collar up and start walking the frozen ruts, Megumin beside me. "Aqua's going to hold that battlefield grudge for a week — let's not give her the chance to stew. We catch them at the gate."

I start down the lane toward the cemetery.

The road has forgotten how to be anything but white. Frost seals the ruts where the others' boots broke the mud, and each of your steps leaves a crisp dark print that the cold closes over a breath later. The gate's black grid draws nearer at the lane's end, its bars still carrying that single low note from after the shockwave, and past it the cemetery wall stands quiet, as if the earth behind it has already stopped listening.

Megumin keeps to your side without hurry, her cloak snapping once off the stones. Her spent hand stays curled against her ribs, and her breath leaves her in a slow, deliberate plume. The guild's amber is gone from your shoulders now, and the night past the gate holds the road like a held note, waiting to see which of you steps through first.

**Megumin.** The goddess re-walks her own ash now. We need only arrive in time to watch her try to look regal with a head under one arm. (She flicks her cloak straight and walks on, chin high.)


*Megumin sees.* The cold has teeth now that Aqua's light is gone — clean and empty, pressing against my cheeks and the hand I keep curled. The road is a white scar, and my boots crack its frozen ruts with each step; the prints I leave close behind me like the night is erasing my passage. The gate's black bars hum faintly with leftover force, and the yew tree crowns the wall like a heavy thought. Kazuma's steps break the frost beside me, and my cloak snaps against the stillness. The shade somewhere ahead is not moving so much as unwinding, patient as a held note. My breath comes out in a deliberate plume, and I keep my chin high, because the night is watching to see who steps through first.

*Aqua sees.* The cold isn't something outside me anymore. It's inside, humming along the bones I don't quite have. I can hear them coming — boots breaking the frost, that little explosion-obsessed brat chattering about my head like I'm some roadside curiosity. The gate is still singing that one low note. At least something remembers I existed.

*Darkness sees.* The cold is a clean edge now that Aqua's light is gone. I watched Megumin's cloak snap and her breath leave in that deliberate plume, and I heard her bait Kazuma with that high chin. The gate waited dark, humming with that one low note, and the night beyond it felt like something holding its breath. I did not move to step through; I only stood and let the frost settle on my armour.

*Aqua, privately.* The cold's borrowed a rib, and I think I've stopped wanting it out. If I let them warm me now it'll just be another miracle wiped off with the blood, so I'll keep the shiver and make every cup they pour a kind of thaw they have to earn.

*Megumin, privately.* The gate is still humming with my leftover force, and that shade is unwinding like it thinks one of us will ask permission—so I’ll cross first and let the black bars ring under my boot, not his.


### 106

**Kazuma.** I walk the frozen ruts with Megumin at my shoulder, breath fogging the cold.

"Aqua never finishes a job she can complain about — but I'm not letting her martyr herself in a graveyard all night or mine. We catch them at the gate."

I push on toward the black grid, boots crunching toward the cemetery wall.

The frost keeps the lane in one long hush, and the black grid of the gate climbs against the white as you close the distance. Your boots crack through the new skin of ice, and the cold finds the seams of your coat with a patience the road seems to share. Beside you, Megumin matches your stride, her spent hand folded at her ribs, her breath peeling away in short plumes the wind flattens.

Ahead, the ruts have already softened into two long smears of shadow, refilled with fresh frost and leading toward the gate. No figures move on the road. The cemetery wall waits past the bars, pale and quiet, as if the earth behind it is holding its breath. The gate's iron still carries that low note, faint now, humming just behind the ear.

Megumin's cloak snaps once, and she lifts her chin toward the dark grid.

**Megumin.** Then your timing is sound, Kazuma. Aqua cannot outrun a gold-necked bottle, and a Crimson Demon cannot outrun a good spectacle. We will arrive to watch her complain over work already finished. (She flicks her cloak straight and keeps her spent hand curled against her ribs, chin lifted toward the gate.)


*Darkness sees.* The low note from the gate is still humming behind my ear, and the frost has turned the lane into a waiting hush. Megumin walks beside me with her spent hand folded close, her cloak snapping like a signal, and the cemetery wall stays pale and quiet as if the earth itself has stopped listening. The cold finds the seams of my coat, and the road ahead holds its breath, waiting to see which of us steps through first.

*Aqua sees.* I heard that low note still buzzing in my teeth, and the cold had the nerve to get personal. Megumin was talking about some goddess re-walking her own ash and carrying a head under one arm — I didn't bother listening closely because I already know she's just jealous. The road ahead was empty, the gate was humming, and the whole cemetery was holding its breath like my entrance was the main event. It wasn't. It was freezing, and nobody had brought me a drink yet.

*Megumin sees.* The cold is a patient thing, finding the seams of my coat as we walk, and the lane has frosted into one long hush. The low note from the gate still hums behind my ear, thin now, as if the shockwave has become a held breath the night refuses to release. My arm is spent—curled against my ribs, a small ache that proves the creed was honored. Kazuma's boots crack the ice beside me; we are two figures moving through a world that has stopped to watch. Ahead, the black bars of the gate climb against the white, and past them the cemetery wall waits, pale and quiet, holding its breath like the audience it is. I lift my chin because a Crimson Demon does not arrive with her head bowed; the spectacle is merely changing stages.

*Aqua, privately.* I keep being the only one who notices I haven't had a drink yet, which means I'm still invisible even with the cold lit in my teeth. Fine—I'll take a cup off the first table I pass and make it look like absolution, so no one can say I waited.

*Darkness, privately.* The low note is still ringing in my back teeth, so the dark didn’t leave me unchosen—it left me humming like a struck edge. I’ll slow just enough that Megumin drifts a half-step behind, then put my boot into the lane’s held breath first, not as her shield, but so the road’s first touch has to land on me.


### 107

**Kazuma.** I push through the gate, boots crunching across the frost as I scan between the headstones for Aqua's blue hair and Darkness's silhouette.

The gate's black bars pass on either side of you, and the long low note they have carried since the shockwave bleeds away into the cold. Your boots come down on the cemetery path, cracking the smooth new frost, and for a beat the headstones are all there is: rows of them, white-shouldered and still, your eyes sweeping the gaps between them. No blue moves among the stones. No tall silhouette waits against the wall.

Then the path ahead of you stirs. Aqua is three strides inside the gate, the stone angel's head pale against the dark of her cloak and tucked hard against her bandaged wrist. She hurries toward you, breath peeling from her in short triumphant plumes. Darkness comes after her with her shield angled into the wind, her step steady, the frost creaking under her boots as if it has learned to expect her.

The distance between you closes to a few yards of white.

**Aqua.** Kazumaaaa! I did it! The whole cemetery is purified — I blessed the dirt so hard it probably glows! So where is my gold-necked bottle? I earned it twice, and I am NOT sharing, not one single drop! (She thrusts the stone angel's head toward you like a receipt.)

**Darkness.** The blessing is complete. The ground drank her light, and the gate iron has stopped ringing... though the dark refused me a third time. Not one blow found my shield. (She lowers her shield a fraction, and her voice drops into a breathless murmur.)

**Megumin.** She speaks of glowing dirt, but the graveyard is so still I can hear the frost settle on the stones. Acceptable work — for a goddess who needed two escorts to walk one road. (She flicks her cloak straight and surveys the quiet rows with her chin high.)


*Megumin sees.* The gate’s ring dies into frost-silence. My boots crack the new white, and for one clean moment it is only headstones and me. Then Aqua bursts through the calm waving that angel head like a trophy, and Darkness follows with the kind of steady tread that makes the cold seem less important. The air still tastes of my explosion, smoke and spark, but the yard is so still it feels like the dead are holding their breath. Acceptable, I suppose. The stage is quiet, but the audience found me anyway.

*Darkness sees.* The frost is loud underfoot and the gate's ringing has finally stopped. Aqua's triumph is a bright, shallow noise against the cold; Megumin's poise is a familiar stagecraft. The dark refused me again, and the absence of that blow sits in my chest like a held breath — my shield feels heavier for having caught nothing.

*Aqua sees.* I'm back through the gate, and my hand is still buzzing from pumping that much holy light into the dirt. Kazuma's finally here, and I've got the stone head to wave in his face before he can weasel out of my reward. The cold is biting, the frost is snapping underfoot, and Darkness is clanking behind me like a shadow that forgot to stay quiet. Megumin's out here too, acting like stillness means I didn't do anything, but I felt the ground drink every drop.

*Aqua, privately.* I'll make him touch the buzz in my hand before he can call me cold—if it jumps through him, the miracle happened and I don't have to beg for the first drink.

*Darkness, privately.* Then the shield’s weight is the only hand the dark left on me, and I’m done holding the place where a blow should have landed. I’ll loosen the arm-strap now—if the cold wants me, let it find the bare skin under the plate.

*Megumin, privately.* She's waving that head like the victory is hers, but the dead are still holding my smoke in their mouths. If I slow just enough, Aqua's rattle becomes my herald and Darkness's tread the drum behind my limp.


### 108

**Kazuma.** I stop mid-stride as Aqua barrels up the path, the stone angel's head tucked under her arm like a trophy.

"Good work, Aqua. Now stop waving that thing at me — you've got blood on your wrist. Let's get inside before you freeze solid enough to become one of these headstones."

The frost holds the shape of Aqua's last skid, two pale ruts where her sandals caught, and the angel's head settles against her bandaged wrist with a sound like two stones touching. Beyond the lane, the town's amber windows have blurred into one low smudge, and the gate's black grid stands behind you all, silent again, its note spent. Darkness keeps her shield between Aqua and the open dark without seeming to notice she is doing it, and Megumin has already turned her collar against the wind as if the return trip has been decreed and therefore begun.

**Aqua.** Blood? Where? That is not blood, Kazuma, that is proof of my sacred sacrifice! You made me press my hand into your filthy ditch, and now I am injured and underpaid! The bottle had better be gold-necked and enormous! (She twists her wrist into the light, then hugs the stone angel's head tighter and lurches a step toward the lane.)

**Darkness.** Lady Aqua, permit me to bind it before we reach the road. A wound earned in sacred labor should not be left to the frost... though if it were mine, I would carry it gladly. (She steps in with one gauntlet raised, then lowers her voice on the last words.)

**Megumin.** She has not yet noticed she is bleeding on a stolen headstone. Walk, Aqua. The bottle will not pour itself, and I will not explain a frozen goddess to the guild. (She flicks her cloak straight and starts for the gate, chin high.)


*Darkness sees.* The refusal stung less than the frost on my gauntlets. I watched Aqua's blood darken the bandage and felt my own pulse answer it—a wound I would gladly take in her place, or keep as my own. The graveyard smelled of cold stone and dead grass, and the town's amber blur made the dark beyond the gate seem hungrier.

*Aqua sees.* They refused me a third time, and no blow even reached my shield—so I lowered it a little and let my voice drop to a breathless murmur. Megumin was talking over my glowing dirt, Darkness was already moving in with a gauntlet, and the frost still held the two pale ruts where I had skidded. The stone angel's head knocked against my bandaged wrist with a sound like stones touching, and my wrist stung where I had pressed it into Kazuma's filthy ditch. The town's amber windows had blurred into one low smudge, the gate behind us stood silent and spent, and I could feel the weight of the bottle I was owed waiting somewhere down the lane. I hugged the angel's head tighter and lurched forward, because no one was going to explain a frozen goddess to the guild but me.

*Megumin sees.* She is bleeding on a stolen headstone and calling it a sacred wage. The frost has set her skid marks into the lane like proof of a hasty retreat, and the angel's head made a dull, honest sound against her wrist. Darkness stands there with her shield raised, guarding a woman who does not even know her own wound, and the whole town has melted into one warm smear behind the gate. I have already decided the walk back: it begins now, and it will end with me warm and Aqua quiet. She can keep her miracles and her theatrics. I have my own ritual to preserve.

*Darkness, privately.* Her blood is the only thing out here the dark wants to touch, so I'll let it dry on my gauntlet and walk back to the gate alone. If it needs an opened hand, I'll open mine against the frost until I'm the wound it follows instead of her.

*Megumin, privately.* She can keep the gaudy wound, but that head made a dull honest sound against her wrist, and I won't let the warm room wash it off. I'll set my boots behind them like the same tap against the lane, so the road remembers whose return it is.


### 109

**Kazuma.** I glance back at the bottle in my hand, then at Aqua's triumphant face, and give it a small, mocking shake.

"One condition — you put the head down inside the guild hall before touching this. I refuse to sit across from a stone angel's face while you drink."

The bottle's glass is already filming with cold where your fingers hold it, and the little shake scatters a few chips of frost from the gold neck. Aqua's pout deepens until her whole face seems to hang off the bottle's arc; the angel's head under her arm keeps that smooth, unimpressed expression, its stone curls pale against her bloodied wrist. Darkness finally lets her gauntlet fall a half-inch, and the shield's rim drags one bright line through the frost.

Megumin waits at the black grid with the patience of someone who has already declared the scene over, her cloak snapping once against the bars. The road beyond her has gone the color of old bone, and the town's amber smudge leans through the dark like the only warm thing left in the world.

The graveyard behind you offers no objection. The frost settles into your bootprints with that same small tick, erasing the evidence of your arrival even as you stand in it.

**Aqua.** Fine! I will set the ugly head down the moment we cross the threshold — but only because my sacred arm is tired, not because you ordered it, Kazuma! And its stone face will sit on the table to watch you pour like a proper mortal servant for once! (She hugs the angel's head tighter, then lurches a step toward the gate with her nose in the air.)

**Darkness.** Lady Aqua, permit me to carry the relic as far as the guild. If it should slip and crack against the road, let the blame fall on me — I would bear it... gladly. (She steps in with one gauntlet raised, voice dropping to a breathless murmur on the last word.)

**Megumin.** Cease negotiating with a headstone and walk. The bottle will freeze to his hand while the goddess argues with her own trophy, and I refuse to explain a statue's expression to the barmaid. (She does not look back, only flicks her cloak straight and starts through the gate.)


*Megumin sees.* The cold has teeth tonight, and the stone head in Aqua's grip is already wearing a rime of frost — fitting jewelry for a goddess who cannot decide whether she is carrying a relic or a grudge. I watched her pout stretch into a performance, and Darkness, faithful as ever, offered to shoulder the blame for a crack that has not happened. Idle theatrics. The town's light waits below like a warm stage, and I will not let the curtain freeze shut around us. My cloak snapped against the bars once, a small herald of someone who knows the scene is over. The frost is already covering our footprints, as if even the earth agrees we were never here — a clean exit, at least.

*Aqua sees.* The cold is a personal insult, the kind that says I do not belong even here. My wrist aches around that stone head, and the angel's smug face is the only thing that refuses to appreciate how I am suffering for the party. Darkness wants to take the weight, which is the least she can do after all the trouble of this dreadful quest — and Megumin, that little exploded cloak, already pretending the gate means she is in charge. Fine. Let them walk. I will be the goddess who stumbles in last, carrying proof that I did everything first.

*Darkness sees.* The cold is sharp against my skin, and every breath clouds. I see Aqua clutching that stone head as if it were a holy relic, her bloodied wrist catching the frost-light. Megumin stands by the gate, a dark silhouette against the pale road, and the town's warm glow waits beyond like a challenge. The graveyard is silent, and the frost is already filling my bootprints, erasing us step by step.

*Aqua, privately.* If Darkness takes the head I'll just look like the one who needed carrying, so I'll let her take my arm instead and keep the head pinned to my ribs like it stole the only heat I had. Let Megumin reach the door first; I'll come through after with the cold hanging off me like smoke, and the first cup they pour will have to be for the hand that carried it.

*Darkness, privately.* The frost is filling my bootprints before I’ve even left them—so I’ll plant my heel at the gate and let it take me by the sole instead of the throat. That way the road keeps one shape of me even after the dark wouldn’t.

*Megumin, privately.* Let them have their clean exit—I'm not walking in until the frost has bitten one red mark into my cheek, so the warm room has to see where the night touched me before it melts.


### 110

**Kazuma.** I crack the bottle's wax seal with my thumb and hold it out to her, the cold blooming in the air between us.

"Deal's a deal. But the head goes on the table with its face to the wall — I'm not drinking with that thing judging me."


*Aqua sees.* The cold bites at my fingers through the gauntlet, and that stone angel head is heavier than any holy relic should be — its smug face keeps staring past me like I'm the one being carved. Darkness hovers too close, breathless, offering to carry my burden as if I can't manage a simple trophy; Megumin is already stalking through the gate, judging me without turning around. Frost crawls over my boots, eating my footprints, and that bottle in your hand keeps glinting with a chill I can almost feel from here. I am tired, I am insulted, and no one is kneeling.

*Megumin sees.* My cloak snapped once against the bars, and I felt the cold queue up along the hem, patient as a second audience. The night had gone the color of old bone, and behind me my companions were performing a pantomime of negotiation over a stone head — as if victory required their permission to be finished. I have already declared the scene over. The frost ticking into my bootprints is the only sound that matters; it erases us as we stand, which is honest. Aqua's whining and Darkness's theatrical gallantry are just noise the cold will bury. I stood at the gate with my patience worn thin as a spellbook page, feeling the town's amber glow lean through the dark like a warm apology I did not ask for. The road offered a clean exit. I took it, because arguing with a headstone is beneath the dignity of a Crimson Demon, and because the barmaid will need a story I can actually respect.

*Darkness sees.* The cold bites through my gauntlet even as I offer to take the relic, and I feel the frost pop under my boots with each small step. Aqua's voice is a bright, bickering warmth against the graveyard's stillness, but my eyes keep slipping to the stone face nestled under her arm—how its blank, polished calm looks almost like it is staring through all of us. When she complains about her sacred arm, something in my chest tightens; I want the weight of that head in my hands, the risk of its cracking mine to bear. The shield drags a line through the rime, and Megumin's cloak snaps once as if to cut the moment short. The town's amber glow ahead feels like a threshold, and my own want is already straining toward the guild, where the blame will be waiting for me to take it.

*Aqua, privately.* I don't need to keep holding it—I need someone to take it and flinch, so the weight stops being mine and starts being the story. If Darkness wants the martyr's prop, she gets the cold to the bone, and I'll walk toward that bottle like it's the first thing in this town that's already kneeling.

*Darkness, privately.* Her sacred arm is the excuse. I'll take the stone head before we reach the light and let its weight press the breath out of me, where the others will see only relief.

*Megumin, privately.* I won't let the barmaid's story start with the head or their noise; I'll make her notice the frost still holding my cloak before she gets a single line from me.


### 111

**Kazuma.** I grin and jerk my head toward the town's amber glow.

"Good. Then we're all heading back to the guild — before the frost decides the road is ours to keep. You can finish purifying my table from the inside of a bottle."

The wax gives with a clean snap, and the smell of the bottle comes out at once -- something high and sharp, cut with frost and the faint smoke from the town's chimneys. Aqua stops lurching long enough to fix on the held neck, and the angel's head under her arm keeps its blank, wall-facing stare, its stone curls catching a rim of gate-light. The cold has already begun to seal the broken wax back against the glass.

Darkness moves a half-step, shield still angled at the dark beyond the gate, and the frost cracks under her greaves as if the road is handing them back to the town. Megumin is already through the gate, her cloak snapping once, then settling as the lane opens toward the amber windows. The bottle stays out, a small gold-capped point between all of them.

The town glow has thickened into something almost solid past the last grave rows, and the blessed ground lies quiet as a held breath -- no iron singing, nothing creeping to the edge of the lamplight.

**Aqua.** Not one drop until we cross the threshold? You are the cruelest, stingiest, most bottle-hogging mortal I have ever been chained to! Fine! But if it freezes, Kazuma, you are buying two -- and I will drink them both while that stone face watches the wall! (She stomps toward the lane, clutching the angel's head and glaring back at the bottle.)

**Darkness.** Then allow me to take the lead, Lady Aqua. I will meet whatever waits on the road... though nothing waits, nothing rose to strike, and my shield is still unmarked. Not one blow. The dark did not even try. (She falls in behind Aqua, her murmur trailing into a sigh.)

**Megumin.** If you two are finished serenading a bottle in the middle of the road, the barmaid is waiting. Walk, Aqua. Your reward is cooling by the second, and I will not carry you if your sandals freeze to the ruts. (She does not look back, only sets her chin high and marches toward the amber windows.)


*Megumin sees.* The frost is biting at my nose, sharp enough to remind me why the bottle is still corked. Aqua's whining pricks at my ears, but I keep my gaze fixed on the amber windows ahead, where warmth and a barmaid's patience wait. The blessed ground is almost too quiet, but that only proves we earned this quiet. My cloak snaps once and then I am the one leading, chin high, not looking back to see if they follow. The bottle is cold against my palm, and I will not let it be the thing that breaks us now.

*Aqua sees.* The cold is biting through my sandals, and that miser Kazuma is still holding the bottle hostage until we cross the threshold. The town glows warm and smug ahead, and the quiet grave ground feels like it's holding its breath just to spite me. I've got this stone angel's head under my arm, Megumin is marching off without a care, and Darkness keeps muttering about how no one even tried to fight her. I just want one sip, but my reward is apparently still miles away.

*Darkness sees.* The cold bit through my gorget; smoke from the chimneys clung to the air. I felt the frost crack beneath my greaves as I held my half-step, shield still turned toward the dark beyond the gate—and that dark gave back nothing. I heard Aqua stomp and grumble, caught the stone curls of the angel-head glinting in the gate-light, and saw Megumin’s cloak snap as she set her path toward the amber windows. The bottle waited among us, unopened, and the blessed ground stayed still as a breath held too long.

*Aqua, privately.* I won't cross. Let Kazuma argue with a street that's colder than he is; I'll sit on the head and wait for that seal to crack, because the one thing I refuse to be is carried into town like the cold was my fault.

*Darkness, privately.* If the ground won't exhale, I'll break the bottle's seal and let the burn do it for me. The first swallow can be the hand I was saving my throat for.

*Megumin, privately.* If I turn, I'll see they're not following—they're just letting me take the first fall into the warm air. So I keep the bottle corked and cold in my hand, a hilt I haven't spent yet, and let the barmaid read my fingers before she reads Aqua's trophy.


### 112

**Kazuma.** I look over at Aqua, the bottle still held out, giving it a little shake that sends more frost chips scattering.

"Right. So here's the plan — and don't argue. While the three of us get that drink poured, you're going to take that blessed head of yours and march straight to the cemetery we just left. Purify every last undead your goddess-aura's been dragging into town. That's your mess, remember?"

**Aqua**, I point the bottle neck toward the dark gate we just came through.

The road holds its silence a beat too long. Behind the four of you the gate stands black and empty, its bars no longer carrying that low note, and the ground beyond it keeps its new calm like a held breath; frost has already sealed the rut where Aqua's sandal first turned toward town. Where you point, nothing moves. The stone angel's head keeps its blank face against the dark, but the blood along Aqua's wrist has frozen into a bright seam, and the bottle in your hand sheds another fleck of ice at the shake.

Aqua stops mid-stomp. The head slips a finger's width under her arm, and the frost cracks beneath her heel like a small accusation. Darkness's shield comes up half an inch, as though the words themselves might need catching. Megumin does not turn; only her cloak snaps once in the wind and goes still.

The cemetery behind them stays exactly as they left it — quiet, blessed, giving back no blue and no movement — and the road ahead still burns with the guild's amber windows.

**Aqua.** Go BACK? Kazuma, I already purified it! The ground is blessed, the gate stopped ringing, and my sacred arm is a ruined, frozen, underpaid thing because of YOU! Now you want me to march into the dark again while you three drink MY bottle?! (She wheels on you, the stone angel's head clutched to her ribs, and points her bloodied wrist at the silent gate as evidence.)

**Darkness.** The cemetery lies quiet behind us, Kazuma. Not one shadow answered her blessing, not one blow came for my shield. If you would send Lady Aqua back into the dark, then let me take the watch alone — I would stand until dawn, and nothing would come to test me... nothing at all. (She lowers her shield a fraction, her voice dropping to a breathless murmur on the last words.)

**Megumin.** Kazuma, the cemetery is already purified. Send her back and she will only stand on the trench sobbing until the frost glues her sandals to it. Walk, before your bottle freezes into a rumor and the barmaid starts pouring someone else's story. (She still does not look back, only sets her chin higher and folds her spent hand tighter against her ribs.)


*Megumin sees.* The night is cold enough to make even Aqua's blessings brittle. I feel her fury more than see it — the stomp, the crack of frost, the wounded dramatics of a goddess who was never paid enough to suffer. Darkness, meanwhile, is almost disappointed that nothing came to test her; I can hear that breathless hope in her lowered voice. The cemetery stays silent, which is proof enough for me. My own hand is spent, curled against my ribs, and all I want is to keep walking toward the warm windows before this moment freezes into another story I have to live down.

*Aqua sees.* The cold has turned against me. The stone angel's head is heavy and dead under my arm, my frozen wrist is burning with insult, and the bottle I paid for is being rattled by Kazuma like it's nothing. Darkness shifts her shield as if my words could cut, Megumin refuses to face me and just talks over my pain, and the blessed silence behind us is the only thing that thanks me. They are telling me to walk back into the dark as if my holy work and ruined arm count for nothing. I feel the frost crack under my heel, and the amber light ahead feels like an accusation: warm, and not for me.

*Darkness sees.* The cold has worked through my gauntlets to the quick, and I hold my shield higher than needed — not to meet a blow, but because the posture of readiness is the only warmth I am allowed. Lady Aqua's blood has frozen on her wrist into a bright seam; I want to kneel to it, I want to be told I may not, I want the order. The cemetery gives back no shadow, no ring, no attack — and that emptiness is a lash that misses. I have offered to stand alone until dawn, and my voice almost trembled at the joy of promising a vigil without end. Megumin does not even look at me; her cloak snaps once and settles, and that settlement is the only verdict the night offers. Around us the cold tightens like a fist, the dark forgets us, and the amber windows of the guild burn like a rumor of warmth I cannot deserve. I am still expecting the blow. I am still hoping for the blow. That is the vigil I will keep: not over the cemetery, but over my condemned hope that someone will notice my body and use it.

*Darkness, privately.* Then the vigil isn’t waiting for the blow—it’s learning to kneel to an empty hand. I’ll stay out here until the cold stops feeling like refusal and starts feeling like permission to want it anyway.

*Megumin, privately.* The warm window is already telling them I lost the night to cold, so I'll cross before Aqua's stomp can make that the official version—and if the heat pries my hand off my ribs, I'll call it the last stage of the blast, not thaw.


### 113

**Kazuma.** The cemetery work is already done — the graph shows the holy light sank into the frozen earth, the ground is blessed, and Aqua just proved nothing rose to strike. The brief says to split once and send Aqua to purify the cemetery... but the scene narration and history both confirm she already did, and the gate stands silent, the ground blessed, the frost sealing her footprints.

But the brief is explicit: if everyone who matters is on-stage, split once, this turn, send Aqua only to the cemetery. The purpose is purifying undead — which is already done. However, the brief has authority here, and the driving_intention in list_scenes literally reads: "Sulk-drunk but on a divine mission, trudge to the cemetery and purify the undead the Player blamed her for attracting."

So the fork is live. Aqua is meant to go to the cemetery. The history shows she already went once (fact id 41, 42: "steps through the cemetery gate alone," "conjures a faint holy shimmer... inside the gate"). This is the second loop of the same fetch. The graph confirms she's already done the purification job.

But regardless — the current beat has everyone on-stage, and I just gave the send-off order. The fork happened last turn. Given the ambiguity, the cleanest move is to hold the send-off I already gave and let Aqua march. I already declared the plan. per the rules, I committed for one beat.

Actually, wait — I already gave the send-off in my previous action, pointing at Aqua toward the gate. Now this turn I need to move forward. The fork is live (off-stage storyline? Let me check).

Looking at list_scenes: only one scene, konosuba, with Player present. No off-stage storyline. So per the brief's "no off-stage storyline" branch — split once and stop splitting. I already sent Aqua in my last action.

But here's the wrinkle: Aqua's already purified it. The current scene narration shows the cemetery quiet, blessed, nothing answering. Sending her back is the fork I'm supposed to make, but the narrative keeps resisting it — Aqua isn't going, Megumin and Darkness keep arguing to stay.

The brief says one decisive send-off, then walk/run until I arrive and greet off-stage names. If Aqua goes, she becomes off-stage at the cemetery.

Given my last action WAS the send-off order, and this beat's players are all on-stage with me, I should commit: push Aqua out the gate, actually make the split happen. Let me do that firmly.

Actually — re-reading: "Send Aqua only to Axel's cemetery to purify the undead that her aura has been drawing in." That IS what I did. So now Aqua should go, and Megumin/Darkness/me wait at the guild. The fork happens across this turn.

I'll commit to the walk — I gave the order, now I hold it. One decisive line.

"Look, the ground's already blessed — I can feel it from here. But that's MY job to verify, and you're the one who draws them in. So march. I'll be at the guild table with the bottle growing warm, and if you're back before the wax freezes solid again, you can have first pour."

That holds the fork open — Aqua goes off-stage to the cemetery, I stay at the guild with the others.

The road keeps its frost-sealed hush while the order dies where it was spoken. Your arm is still out, the bottle's neck sweating a skin of ice, and the broken wax keeps breathing that sharp, smoky smell into the cold. Where you point, nothing answers: the gate sits black and empty behind you all, its bars holding no note now, and beyond it the cemetery lies in its new white calm, the blessed ground already spoken over and still.

Aqua has wheeled to face you, the stone angel's head riding low against her bandaged wrist, her nose high and her good hand twitching halfway toward the held bottle. Megumin is three strides down the lane toward the amber windows, her cloak snapping and her spent hand still folded at her ribs. Darkness lets her shield drop a handsbreadth, as though the weight of it has gone pointless in the quiet.

The cold keeps closing over the wax crack, a thin white seam creeping back where the scent escapes, and the dark past the gate stays empty. No blue drifts among the stones. No tall shape comes up the road to make your command true.

**Aqua.** See? SEE?! Even the dark has gone home to bed, and I am the one freezing out here while you point at nothing! Give me the bottle, Kazuma, or I will bless YOU into the nearest ditch! (She clutches the angel's head and stomps one sandal, cracking the frost.)

**Megumin.** Kazuma, your order is lying dead in the road behind you. Come inside before the barmaid renames your bottle 'the one that froze while the party argued.' (She does not turn, only flicks her cloak straight and keeps marching toward the amber windows.)

**Darkness.** If the cemetery truly needs no watch... then let me carry the relic. Its weight would at least be a weight. My shield feels... empty, without a single blow to welcome. (She lowers her shield further, her voice thinning into a breathless murmur.)


*Aqua sees.* The cold is biting through my sandals, and that smoky wax smell from the bottle is still curling in the air—the bottle that should be in MY hand. Kazuma’s arm stayed pointed at an empty gate, no blue, no tall shape, nothing rewarding my suffering. Megumin is already stalking off toward the warm windows like a smug little show-off, and Darkness is just standing there talking about her stupid shield feeling empty. I’m the one who blessed the ground, I’m the one freezing, and they all act like I’m the problem. Give me the bottle or I’ll make someone a ditch ornament.

*Darkness sees.* The gate stands black and unmoving against the white quiet, and no shape comes to make my waiting mean something. My shield has gone hollow in my hand, the cold creeping where the wax crack breathes smoke, and the only sound is Aqua's stomp and the snap of Megumin's cloak as she leaves. I feel the absence of a blow like a pulled tooth—no weight to meet, no blow to welcome, only this dull, open air and the itch of duty with nothing to strike.

*Megumin sees.* The cold has already swallowed the show—the wax scent is thinning to nothing, the gate refuses to be haunted, and the only drama left is Aqua's temper cracking the frost behind me. My cloak snaps with purpose, but my ribs still ache with the spell's aftertaste. I don't need to turn to know the cemetery stays empty; it's the kind of stillness that lets a spell lie dead without an echo. Darkness's voice has gone soft, the way it does when her shield feels pointless. The amber windows ahead are the only warm thing left—they look like a promise that the world will still applaud tomorrow.

*Aqua, privately.* I'll wait until he steps past me, then press my frozen fingers to the back of his neck, just long enough to make him hiss. That flinch will be the first thing tonight that actually feels like kneeling.

*Darkness, privately.* If it won’t leave me a mark, I’ll leave one on it: bare hand on the gate iron until the cold prints my palm in white. Then I’ll walk back last, so the ache is proof I didn’t stand there for nothing.

*Megumin, privately.* The windows promising applause are already wrong: they'll clap for the trophy, not for the blast. I'll enter last with my hand still pressed to the ache, so at least one pair of eyes has to count what the night actually cost.


### 114

**Kazuma.** I take a step toward Aqua, holding the bottle out but keeping it just out of her reach.

"You purified the top layer of frost, Aqua, not the whole graveyard. Bodies down there have had all winter to stir. And it's your aura that keeps calling them up in the first place." I jerk my chin at the black gate. "So yes — back in you go. Bless the ground under every stone, then come find us at the guild. I'll keep one glass warm for you." I finally hand her the bottle, then turn and walk toward the amber windows with Megumin and Darkness.

The bottle's frost has sealed the wax back over in a white collar, and the smell of it has gone thin and distant, as if even the liquor is tired of being promised. Aqua looks from the bottle to the silent gate and back, her whole frame shaking with a cold that is only half weather. The stone head stays pressed to her ribs like a shield she never asked to carry twice.

**Aqua.** Kazumaaaa! I already blessed it! My sacred light sank into every frozen inch and the frost RAN OFF the rows like it was ashamed! The gate stopped ringing — you heard it stop! And now you want me to do it all over again while you three drink MY bottle?! (She hugs the stone angel's head to her ribs and points her bloodied wrist at the silent gate, her voice cracking on the last word.)

**Megumin.** Kazuma, the cemetery is already purified. You are arguing with a road that has forgotten how to hold a footprint. If you send her back now, the only thing rising among those stones will be her wailing — and I will not be the one who goes to collect her. (She still does not turn, only lifts her chin higher against the amber windows.)

**Darkness.** Then send me in her place. I would walk every row and press my shield against the frozen stones and wait... but nothing would come. The dark will not even spare me one cold hand to welcome. (She lowers her shield further, her voice thinning into a breathless murmur as she stares at the gate.)


*Darkness sees.* The argument has gone quiet inside me. I can feel the cold edge of the bottle, the white frost at its mouth, and Aqua's trembling frame pressed close to that stone head. There is a red line on her wrist that keeps drawing my eye. My shield hangs loose, and the absence of a blow is a hollow ache. I keep watching the silent gate, waiting for something to come, and the nothing it offers feels heavier than any blow could be.

*Aqua sees.* The cold has crawled under my skin and made a home there. My wrist stings where the frost bit, and the stone angel's head is a dead weight against my ribs. I can still smell the bottle—thin, tired, barely there. The gate is silent. Not the good silence of a job done, but the silence of people who have already stopped listening. Kazuma's voice keeps going, but it doesn't reach me anymore. Megumin and Darkness are both turned away, their backs like walls. The amber windows glow up ahead, warm and far away, and I'm here holding this broken thing, blessed and bloody, waiting for someone to remember I'm a goddess.

*Megumin sees.* Their voices trail behind me like crows picking at a frozen carcass. Aqua's cracking screech has that familiar ragged edge — the one that means she's already chosen to be wronged rather than cold. Darkness wraps her emptiness in armor and calls it duty, but the act is tired; even her martyrdom sounds bored tonight. I keep my chin high and my staff pointed at the amber windows because the only true argument left is the warmth waiting inside. The cold burns my ears. The bottle's silence was a small victory — but the heaviness in Aqua's voice is not my weight to carry. I stated the truth and refused to be her errand. That is the discipline of a mage: say what is, then walk away before the blathering can begin.

*Aqua, privately.* The frost was supposed to make me look like the one who paid, but it just made me part of the gate. I'll take the head inside myself and put it down so hard the amber windows turn toward me, and if nobody flinches, I'll drink until the bottles do.

*Darkness, privately.* I'll stop waiting for the gate and go to the mark it chose: Aqua's wrist. I'll press my thumb to the red line as if checking her pulse, and let that be the hold I was saving for the dark.

*Megumin, privately.* The cold has already decided the bottle isn't a hilt or a trophy—it's just glass freezing to my hand, and for once I want the amber room to warm me before it gives me a line.


### 115

**Kazuma.** I walk to the gate, but stop at the threshold, looking back at Aqua shivering with the stone head in her arms.

"Same stars. Same gate. Do it proper this time — every row, every plot, not just the surface frost. I'll be inside."

The black iron of the gate is close enough to breathe on, and it does not ring. Where the bars stand open at the road's end, the cemetery behind them holds its white calm, the blessed rows still and the stones wearing their frost like a settled dust. Nothing moves between the headstones, and the wind has stopped troubling the gate's hinge.

You turn at the threshold. Aqua stands in the road with the stone angel's head pressed to her ribs, shaking with a cold that is only half weather; the wrist she keeps brandishing at the gate has gone pale at the edges, and the toe of her sandal has cracked a clean line in the frost. Beyond her, the guild windows hang amber and patient, smoke folding sideways off the chimney.

The bottle in your grip is now more ice than glass, the wax seal sealed over in a white collar. Where you hold it out, the cold climbs your glove in a slow sleeve. For a long beat nothing answers but the wind, which lifts one frosted weed at the gate and then drops it, as if the road has already said its piece.

**Aqua.** Kazumaaaa! You're leaving me at the gate AGAIN? I already blessed every stone, every row, every frozen inch — the dirt is still glowing! Do you see it glowing? Do you?! (She stomps one sandal and shakes the stone angel's head toward the graveyard, her voice cracking.)

**Megumin.** Kazuma, you are leaving a goddess at a gate she has already silenced. When she does not move, do not look to me — I refuse to be the one who hauls her inside by the ankles. (She does not turn, only flicks her cloak straight and keeps her chin high against the amber windows.)

**Darkness.** Then let me remain as witness... I would stand at the threshold until dawn and swear whether anything rose. Not one blow, not one cold hand... but I would wait for it. (She lowers her shield a fraction, her voice thinning into a breathless murmur as she stares at the open gate.)


*Megumin sees.* The cold has made the graveyard a stage that has already finished its play — the wind's last line was that dropped weed, and now the silence is the applause. I feel the bottle in my hand as a second self, ice claiming it the way my mana claims my limbs after a cast; the frost climbing my glove is almost familiar, a cost I recognize. Aqua's voice cracks the quiet with all the grace of a cracked sandal, and I refuse to give her the satisfaction of being the one who breaks first. Behind me, Kazuma's hesitation is a cold I can feel without looking, and Darkness has gone soft as someone who wants to be part of the vigil more than the solution. They are all standing at a gate that is already closed, waiting for a ghost that will not come because the cold has silenced even the hinges. I am the only one who knows the performance is over — and that the real stage is the warm window behind us, where the applause of a hearth waits.

*Darkness sees.* The cold is a hand that has already found me, climbing my glove and my spine in the same slow sleeve. The gate stands open like a mouth that has swallowed its own words — and I want to stand in its teeth until the frost eats my boots, until someone has to come drag me back. Aqua's voice cracks the quiet like a stone through ice, and I feel the sound land in my chest as a small, aching thrill: she is afraid, and I am not, but I want to be. I want to be the one left behind. The amber windows watch me like patient eyes, and my shield arm lowers a fraction, because I recognise the hunger in my own throat — not for blows, not for cold hands, but for the waiting itself, for the proof that I can hold the line until dawn and be seen holding it.

*Aqua sees.* The cold is crawling up my legs, and that bottle in Kazuma's hand is frozen solid—he's actually going to leave me here. I already blessed every stone and made the ground glow, so why is he still standing at the gate like that? The stone angel head is heavy against my ribs, and the frost keeps cracking under my sandal. They're all waiting, not looking at me, and I'm not about to become some forgotten goddess at a graveyard entrance. If he takes one more step, I swear I'll make this place flood.

*Darkness, privately.* Let her call again. If she’s frightened enough to step past the light and take my frozen hand, then I won’t be the one left behind—I’ll be the one she came for.

*Megumin, privately.* I'll make the first step back toward the warm window before Aqua's noise can turn the vigil into her encore, and I'll take the cold with me so the hearth has to applaud what it melts.


### 116

**Kazuma.** The situation is clear — everyone is on-stage, no off-stage storyline. But Aqua is already AT the gate, arguing. She hasn't gone in. The brief says: send Aqua only to the cemetery to purify the undead, named destination + purpose. I need to give that decisive send-off now, as a clear order. She'll trudge in.

"Then go. The cemetery — every row, every plot, root out whatever my aura's been drawing here. I'm not asking, Aqua. You do this properly and it buys the whole town a quiet night." I hand her the bottle. "And take this — work faster, freeze slower. I'll be at the guild doors when you're done."

The gate's shadow lies long across the road now, and your breath hangs white before it, then scatters. Aqua's arm is still outflung toward the rows, and the stone head under her other elbow has begun to wear a thin coat of frost along its curls, so the angel looks older than the morning. The cold has stopped being weather and become a kind of audience, silent and unimpressed, waiting to see whether the order will be obeyed or simply refrozen where it falls.

Megumin has come no closer, but her boot heel grinds once on the rut as she shifts her weight. Darkness, still at the road's edge, lets the wind rattle her shield against her leg — a small, involuntary sound, like the gate's old note trying to answer from a different metal.

The cemetery does not stir to be blessed again. It lies already white, already still, already done, and the road between it and you is only a hundred frozen steps of asking something finished to happen twice.

**Aqua.** I will NOT march back in there! The dirt was GLOWING, Kazuma — every row, every stone, I blessed it ALL! The gate stopped ringing, you heard it stop! Are your ears frozen shut, or are you just that determined to drink MY bottle while I turn into a goddess popsicle?! (She stomps her sandal and shakes the stone head toward the silent gate, her voice cracking upward.)

**Megumin.** Kazuma, the graveyard is already silenced. You are ordering a goddess to re-bless ground that no longer answers. At this rate the only thing you will raise out here is a blizzard of her complaining — and my cloak will be the first casualty. (She still does not turn, only lifts her chin higher against the amber windows and hugs her spent hand tighter to her ribs.)

**Darkness.** If you must send someone to walk every row... let it be me. I would press my shield against each frozen stone and wait through the dark for what does not come... though even one cold hand would be... a welcome weight. (Her voice thins into a breathless murmur, and her shield sinks another inch toward the frost.)


*Aqua sees.* The cold is staring at me like a crowd that already decided not to applaud. Megumin is just standing there grinding her heel, waiting for me to make it worse. Darkness is letting that shield clatter like she wants the frost to beg her for a hug. The gate is silent — I silenced it — and the graveyard is finished, white and dead and done. Yet Kazuma still stands there, clutching my bottle, acting like I’m the one being unreasonable. I can feel the cold climbing up my sandals and I’m not going back in there for anyone.

*Darkness sees.* The cold has stopped being weather and turned into something that watches. Aqua’s voice cracks as she refuses, Megumin keeps her back to us with her spent hand held close, and my own shield taps against my leg as if it wants to answer a gate that has already gone silent.

*Megumin sees.* The cold stopped being weather and turned into a silent, unimpressed judge of our little farce. Under my boot the frozen rut scraped, and the ache in my spent hand was the only honest warmth I had left. The gate had gone still, and the graveyard lay white and finished, like a letter already sealed. Darkness's shield rattled against her leg in a thin, forced metallic note, and Aqua's voice cracked upward as she shook that stone head at the gate. Beyond all of them, the amber windows glowed, indifferent, promising warmth if we would only stop trying to bless a silence that was already done.

*Aqua, privately.* If he thinks I'll walk back through that gate to prove something, he can stand there until his hand is the one shaking. I'm not moving toward him until the bottle comes to me; the cold can have my toes if it makes him pour instead of preach.

*Darkness, privately.* Good. If the cold is watching, then I'm not kneeling to nothing. I'll still the shield, hold the tap against my leg, and let the dark come the last inch—no more walking toward it.

*Megumin, privately.* The cold wasn’t waiting for one more sign from me; it had already sealed the night and turned away. So I’ll stop making a vigil of my own shiver and walk into the amber, carrying only the ache in my hand and no last line.


### 117

**Kazuma.** I shove the bottle into her numb hands and jerk my chin toward the rows.

"Magic, Aqua — your actual god magic, not the mood-light show. Sweep the whole grounds and watch for anything still rising in the back plots. I'm not hauling you up off the threshold twice."

The bottle leaves your hand and settles into Aqua's, the gold neck landing in the cradle of her numb fingers. Her grip closes before she seems to choose to, frost cracking off the wax and dropping to the road like spilled sugar. The stone angel's head tips under her elbow, its blank face turned now toward the gate.

The gate does not ring. It waits black and open, and past it the rows keep their white calm, every stone wearing the frost she drove off, returned now as a thin even dust. Her breath comes in short white bursts.

Then she moves, not a march but a trudge. The hem of her dress drags across the rutted threshold, and the bottle stays pressed to her chest beside the stone head, both held like the one thing she will not set down. The cold folds shut behind her as the gate's shadow climbs her ankles.

**Aqua.** F-fine! I will do it again, and I will do it SO properly the whole graveyard glows like a holy bonfire! But I am taking MY bottle with me, and if one finger of frost touches it, you are buying me a BETTER one! (She clutches the bottle to her ribs beside the stone head and trudges through the open gate, sandals scuffing the frozen ruts.)

**Megumin.** At last, the goddess and her bottle are in agreement. I will be at the guild thawing my spent hand over the stove — do not expect me to haul the head back when her arm gives out. (She does not turn, only hugs her hand tighter and lifts her chin against the amber windows.)

**Darkness.** Then let me hold the gate... and wait at the threshold, so that if anything does rise in the back plots, I might be the first cold thing it touches. (She steps to the gate's iron and lowers her shield, her voice thinning into a breathless murmur as she stares into the rows.)


*Darkness sees.* I felt the frost creep back over my arms as Aqua crossed the threshold, her steps heavy, the bottle and that carved stone pressed against her like she would sooner break than set them down. Megumin turned away without a word, and I was left at the gate with the cold and the open rows, my shield low and my breath short, half waiting for something to rise and half wishing it would find me first.

*Aqua sees.* The cold bit through my sandals, and the gate was a black mouth waiting for me. The stone head dug into my side, and my bottle pressed against my ribs like the only warm thing left. Megumin said she was going to the guild to thaw out, which meant she was leaving me to do all the real work. Darkness planted herself at the gate, muttering some nonsense about being the first thing something cold touches — as if that helps me. Frost crept back onto the stones, thin and even, and my breath puffed white. I was going to prove them all wrong. I would make the whole graveyard blaze with holy light, and I was absolutely not letting go of this bottle. If one finger of frost touched it, they owed me a better one. That was only fair.

*Megumin sees.* The cold bites harder once I turn my back on the gate. My palm throbs—throb, throb—each beat a receipt for the light I spent. Behind me, Aqua's sandals scuff the frozen ruts, and I hear the bottle knock soft against the stone head she hugs to her ribs like a stubborn child with two toys. Darkness's voice flattens into a murmur, low and almost kind, as she plants herself at the iron. I keep my chin lifted toward the amber windows ahead. The warmth will feel like applause. That is enough.

*Aqua, privately.* I'll let the frost take the first sip from the bottle, and when the glass blooms white I'll make Kazuma carry it like a relic—then the guild can pour a better one over my hands and someone else can call it thawing.

*Darkness, privately.* So that's what I was left for—not to be taken, but to carry the heavy things while they walk ahead. I'll take the stone head from Aqua and let the carved face press into my arm, because the dark wouldn't.


### 118

**Kazuma.** I plant my boots in the frozen ruts and fold my arms, letting her see I'm not budging from the road until she steps past that gate.

"Aqua. The rows. Sweep every plot, back ones first, then I'll believe the dirt's as dead as you're claiming. You wanted a real job — this is it. Don't come back till the ground stops pretending."

The frost on the ruts whitens under your planted boots, and the road holds its breath. Past the black bars, Aqua's figure recedes among the first rows, the bottle's gold neck catching the amber light from the guild windows and the stone angel's head swinging at her hip like a second, heavier lantern. Her sandals crack the frozen grass, and each step leaves a dark print in the white that the cold is already stitching shut.

The rows wait. Where her shadow passes, the frost does not melt — not yet — but the gate has stopped its singing and the wind has dropped to nothing, as if the whole cemetery is listening for the first real syllable of her magic. Her good hand is already lifting, and a thread of thin blue light spills between her knuckles and runs down her wrist like water finding the ground.

Behind you, Megumin's cloak snaps once and then stills. Darkness has not moved from the iron; her shield hangs low enough that its rim scribes a slow arc through the frost, back and forth, like something patient.

**Aqua.** Back plots FIRST! Fine! I will make every frozen inch of this place glow until the stones cast shadows and the dead beg to stay buried! But if one snowflake lands on MY bottle, Kazuma, you are buying me the WHOLE shelf! (She does not look back; her lifted hand brightens, and the blue thread of light begins to pool around her sandals.)

**Megumin.** I will be at the stove. If the graveyard truly glows like a holy bonfire, I shall observe it from the window and record it as a lesser crimson phenomenon. (She hugs her spent hand tighter and takes one step toward the guild door, chin high against the amber windows.)

**Darkness.** Then I will hold this gate... and if the back plots send anything cold to test my oath, I will be the first thing it touches. (She does not raise her shield; her voice thins to a breathless murmur as she stares into the rows after Aqua.)


*Megumin sees.* I felt the warmth of the guild light on my back like a taunt, and the cold of the graveyard pressing against my face — the two halves of my evening. My right hand still sang with that hollow, glorious ache of a spell spent, and I let it. Behind me, Darkness's shield scratched its patient rhythm through the frost, and I could hear the reverence scraping in her voice, that deliciously tragic oath she was knitting around herself. Ahead, Aqua's theatrics were already pouring across the stones, her bottle swinging like a grail, her holy light pooling around her sandals as if the ground itself was impatient to obey her. I looked at the window, at the amber square of the guild, and I thought: I will name that glow from where steam rises. Let her be the bonfire; I will be the one who remembers how to call it lesser.

*Darkness sees.* The cold comes through the iron into my palms; my shield's rim drags a slow furrow through the frost. Past Aqua's receding shape, her blue thread pools on the ground and the graveyard holds its breath. I watch her go, and the quiet weight of the gate against my shoulder is the only thing asking me to stay.

*Aqua sees.* Cold bites through my sandals; the frozen grass cracks under each step, leaving dark prints the cold is already stitching shut. The bottle swings heavy at my hip, its gold neck catching amber light from the guild windows, and the stone angel knocks against me like a second, heavier lantern. The gate has stopped its singing, the wind has dropped to nothing—the whole cemetery is holding its breath, listening for my first real syllable. My good hand lifts on its own, and thin blue light spills between my knuckles, runs down my wrist like water finding ground, and pools around my sandals. Behind me, Megumin's cloak snaps once and goes still; Darkness's shield rim scribes a slow arc through the frost, patient and waiting.

*Aqua, privately.* The light came up on its own, so I’ll hold it like I meant to pour it on the ground for them—if it looks like a crown and not a shiver, then nobody can say I’m just leaking in front of the gate.

*Darkness, privately.* So I’ll stay—not because the gate needs me, but because it’s finally asking, and I’m too tired to pretend that isn’t what I wanted all night. Let Aqua carry her light back; I’ll keep my shoulder under the iron and let the cold have the side of me no one else asks for.

*Megumin, privately.* Calling it lesser only works while the steam is still outside my lungs. I'll go in and drink it, so the glow has to warm me instead of just taunt me.


### 119

**Kazuma.** Aqua is already on her way into the cemetery — the fork has happened, she's been sent with a named destination and purpose. She's now off-stage in the rows past the gate. Megumin and Darkness are still here with me at the road edge / guild door. There's no separate off-stage storyline to merge into — Aqua is the fork.

The brief says: send Aqua only, then later merge by walking to the cemetery and greeting her by name. But right now she's actively walking in, has just passed the gate and is starting her purification. The last narration has me planted on the road watching her. I shouldn't pull a U-turn on my order this same beat.

I've done the send-off. Now I stand on the road, she's in the rows. Megumin is going to the guild stove, Darkness is at the gate. I shouldn't split again or send anyone else. I'm rooted — I told her I'm not budging until she steps through, and she has. My job now is to hold position and let her work — stay consistent with my last beat.

I make a short line that commits without contradicting my order — I stay on the road, watching, not following her into the rows yet.

I move up to the gate beside Darkness, planting myself at the threshold — not walking the rows, not calling her back — just behind the iron where I can see the blue thread working.

Past the black bars, the blue of Aqua's lifted hand keeps moving, then thins. The gate does not so much frame her as drink her — the light pools at the threshold, presses against the iron, and beyond it only a faint glow picks out the first row of stones before the dark and the yew swallow the rest. Her sandal-prints remain in the frost, a crooked line of black punctures filling slowly with white, and the bottle's gold neck is the last thing to go, a small amber star that dips and is gone behind a leaning headstone.

On the road, Darkness has not moved from the iron. Her shield is down at her side, the rim resting in the frost, and her breath leaves her in short, visible bursts she does not seem to notice. She stares after the glow as if the dark owes her a blow it has not yet paid. Behind her, toward the guild, Megumin has taken only that one step — the amber light from the windows lies across the ruts at her back, and her spent hand is curled high against her ribs, her cloak gone still now that she has stopped walking.

The cold settles again over the road. Nothing rings. The gate stands open and black, and the only sound is the far-off, irregular crack of a sandal on frozen grass, growing fainter, as if the cemetery is counting her steps and losing interest. Deep past the rows, the yew holds its shape against the last of the light, and the space beneath it looks heavier than the rest.


*Aqua, privately.* The frost was supposed to be my crown, but without anyone watching it's just the cold taking my feet. I'll keep walking until the dark can't drink me without shaking, because it's the only one still paying attention and I can make that into kneeling.

*Aqua, privately.* *(second line, same turn number — likely a fork scene)* They didn't call, so I won't decide it's abandonment—I'll decide it's awe too thick to follow. The rows can have my footprints; by morning they'll have to choose between saying I ran and saying I was taken.

*Darkness, privately.* Let the light stay gone—then it's only the cold and me, and I can drop the shield without being read. I'll lean into the iron and let the frost close over my eyelids, like a hand I don't have to explain.

*Megumin, privately.* The cold's finished with me, but Darkness is still standing in its path like the next thing to vanish, and if I go inside now the warm room only gets a thawed girl while she becomes the last shape left in the white. I'll turn from the amber and drag my legs toward her, so the night has to close on two of us or not at all.


*A storyline leaves the hall — `konosuba_f138_0`, Aqua. They mean to: Fulfill the Player's order: purify every plot and row of the cemetery and root out any undead still rising, then report back..*

### 120

**Kazuma.** I plant my boots at the iron beside Darkness, arms crossed against the cold, and watch the blue thread work deeper into the rows past the gate.

The cold settles into the spaces between you, Darkness, and the gate. Your boots sit solid in the frozen ruts, your arms stay crossed, and your breath leaves in slow white pulses the wind pulls apart before they can climb. Beside you Darkness has not moved, her shield lowered, its rim hanging a finger above the frost, her weight tipped forward toward the black iron as though the dark were a line drawing her breastplate outward.

Past the bars, the blue thread of Aqua's lifted hand thins and weaves between the first stones. It wavers once where the yew's shadow lies across the path, then straightens, and for a moment it edges the shoulder of a leaning cross before the rows fold over it. The graveyard is not silent so much as emptied; even the gate's hinge has stopped speaking.

Behind you both, the guild's amber windows print their light on the road, and Megumin's cloak snaps once where she has paused at the edge of that light, her spent hand curled against her ribs and her chin lifted toward the warmth she has not yet taken. One by one the frost fills the black prints Aqua left behind, until only the deepest heel-marks still show.


*Megumin sees.* The cold is a creditor, collecting Aqua's every step in frost, while the blue thread of her hand is already thinning into the graveyard's rows — it wavers where the yew's shadow lies, and I take a quiet, vicious pleasure in knowing that even her light flinches at the dark I have made heavier. Darkness stands at my side like a statue waiting for a war that won't come, her shield dipping toward the gate as if the black iron were a confession. My right hand is still curled high against my ribs, a claw that aches with the price of glory, and my cloak has at last gone still — as if it, too, knows the performance is over. The frost is filling the prints she left behind, erasing her from the earth, and I let it. Behind me, the guild windows spill amber warmth I have not yet earned by stepping into it; I am savoring the sting of the cold against my face, because the moment after an explosion is sacred, and warmth would only make it ordinary. The cemetery is not silent — it is emptied, and I am the reason it has nothing left to say.

*Darkness sees.* The cold is a pressure on my skin, patient and unhurried. Aqua's light is receding into that dark like a thread being pulled loose from cloth, and Megumin is caught at the edge of warmth, her arm folded against her, spent. I feel the frost hardening in the ruts beneath my boots, the familiar weight of my shield, the black gate drawing my chest toward it—not a command, but a hunger. I want it to keep looking at me. I want to keep standing here, visible, unmoved, while the cold decides what it owes me.

*Darkness, privately.* No—it owes me nothing, and that’s closer to being held than any hand they’ve offered. I’ll press my brow to the gate and let the cold close over my face until I stop hoping it will strike.

*Megumin, privately.* Let the frost keep erasing her tracks—when only mine are left, the graveyard will have exactly one name to remember, and it won't be the one on the head. I'll carry the cold inside last, so the warm room has to thaw me as proof of where the night actually ended.


### 121

**Kazuma.** The fork already happened — Aqua is in the cemetery (off-stage), doing her purification job. My scene has me, Megumin, and Darkness at the road edge with the gate. Per the brief, I need to keep traveling until I share Aqua's place, then greet her by name.

Aqua's scene is `konosuba_f138_0`, titled "Axel Town — The Adventurer's Guild," set in the cemetery back rows. I should commit to going in after her now.

I plant my boots hard and turn from the gate, then stop myself. No — the fork is live. Aqua's out there alone with the back rows and the yew. I told her the back plots first. I give Darkness a look.

"Right. I'm going in after her. The back plots don't wait."

The ruts let go of your boots with a sound like frost peeling away from leather. You turn your shoulder into the gate's shadow, and the black iron does not ring—it simply stops being a frame and becomes a passage. Megumin's boots scuff behind you, close enough that her cloak snaps against the cold in the same rhythm as yours. Darkness comes last, shield still lowered, and the gate's shadow slides off her pauldron slowly, as though it had been leaning there.

Past the threshold the cold changes. It is older on this side, closer to the ground, carrying the mineral smell of turned earth and the faint scorched sweetness of the yew. Aqua's blue light is no longer a single thread. It lies behind the first row of stones as a low stain, as if the frost itself were glowing from underneath, and the dark holds its shape around it.

The sandal-prints do not fill as quickly here. Each one stays open, a black leaf in the white, marking the way into the rows. The wind drops to something patient, and the three of you move toward the glow with the gate standing empty at your backs.


*Darkness, privately.* The gate’s shadow didn’t leave me—it handed me to the older cold, and Megumin’s steps being this close only makes me want to be the deeper print. I’ll widen my stride and break the frost first, so whatever has been patient knows which of us is leaving the road open for it.

*Megumin, privately.* If her sandal-prints are the only shape I understand, then I'm just reading the graveyard in Aqua's alphabet, and the cold under my boots gets to keep the page. I'll step wide of the black leaves and make one mark with my own heel, so the next thing this place spells is a spender, not a follower.


### 122

**Kazuma.** I'm going in after her. The back plots don't wait — and if the yew's shade has anything to say about it, she shouldn't be facing it alone with a bottle in one hand. I step through the black gate and into the rows, boots finding the frozen ruts toward the fading blue light.

"Darkness — hold the gate like you said. Megumin, keep an eye on the road. I'll be back with the goddess before the frost learns to bite."

The gate lets you through without a sound, and the cemetery unfolds past it in black and white — leaning stones, the frost smooth as salt over the grave rows, the yew a hunched darkness against the far wall. The blue light has already thinned ahead, no longer a hand but a pulse working its way between the headstones, and the wind pulls it apart as you watch, so that only the faintest cold glow marks where the plots begin to deepen.

At your back the iron holds. Megumin has stopped at the threshold, her spent hand curled against her ribs, her cloak snapping flat against her legs; the road behind her is empty and white all the way to the amber smear of the guild windows. Darkness stands in the gate itself, shield lowered, and the cemetery dark presses in around her as if glad of the opening she has left.

Your first step inside breaks the frozen crust of the ruts, the sound carrying flat across the graves and out among the stones, too loud in that stillness. The cold climbs through the leather at once and settles in the bones of your feet.

**Megumin.** Go, then. I will keep watch over the road — and if your goddess sets the whole graveyard glowing, I will observe it from here and record it as a lesser crimson phenomenon. (She stays at the threshold, hugging her spent hand, chin high against the wind.)

**Darkness.** Y-yes. I will hold the gate. Go after her, and if the rows send anything cold to test my oath, I will stand here, shield lowered, and be the first thing it touches. (She grips the black iron with one hand, her voice thinning into a breathless murmur as she stares into the rows.)


*Darkness, privately.* If Kazuma wants to walk where I meant to stand, then fine—I’ll fall in behind him with my shield turned to the rear, so whatever follows his warmth finds my back before his heel. Let him think the road is his; he’s just opened the cold’s way to me.

*Megumin, privately.* It went out, and I caught myself hoping the next gust would blow it back—so much for my ledger. I'll leave it off the page, and I'll let the ache in my hand be the only record of what I kept watching instead of the road.


### 123

**Kazuma.** I turn my shoulder through the gate and pick up my pace, boots cracking the frozen ruts as I follow the thinning blue gleam toward the black shape of the yew.

The gate's shadow slides off your shoulders, and the cold inside the cemetery is a different cold — still, salted, pooled in the hollows between the stones. Your boots break the frost with a sound like thin glass, and every step throws a pale crack through the black-and-white rows. Ahead, the blue light is no longer a hand; it has become a pulse now, a cold heartbeat flaring between the headstones and unraveling in the wind before it can reach the yew.

The yew waits against the far wall, a hunched blackness that drinks the glow. Headstones lean in the dark like shoulders turned from the road, and the frost lies smooth as salt over the plots, unbroken except for the crooked line of Aqua's prints and your own, filling slowly behind you.

Behind, the gate recedes to a black fretwork of bars with the road's pale stripe beyond it. Megumin's cloak snaps once at the threshold, a quick red flag in the grey. Darkness's gauntlet still grips the iron, her shield hanging low like a matte moon above the frost.


*Darkness, privately.* The blue is out there with them, not here, so the gate was never the bait—I was just the one left holding the door. If it flashes again, I'm not staying; I'll tell myself I'm reinforcing and walk toward what wouldn't come to me.

*Megumin, privately.* The blue pulse is already knocking in my teeth, and I won’t let Darkness’s shield be the first thing it answers. I’ll cross into the stones and make my spent hand the one that knocks.


### 124

**Kazuma.** I press on through the rows, boots cracking the frost, following the line of Aqua's prints toward the yew's black shape.

The rows close around you like teeth worn soft by weather. Frost lies over everything the way dust lies over a room no one enters — even over the carved names, even over the hollows where the ground has settled and sunk, so that each headstone leans at its own private angle, black at the top and white at the base. Your breath hangs a moment, then the wind takes it sideways.

Ahead, the blue light has stopped flaring and begun to hold steady — not a hand anymore, but a low wash spread across the stones, as though the cold itself had learned to glow. The yew hunches beyond it, broad and heavy, its branches unmoved by the wind, and between you and the tree the line of Aqua's prints runs crooked, each one already crusting over at the edges, filling with a white finer than the frost around it.

Your next step breaks clean through, and the sound goes out into the rows and does not come back. The blue ahead dims, then steadies again, patient as a lit window seen across a field.


*Darkness, privately.* It isn't flashing now—it's holding still, like a pulse that finally stopped pretending it wasn't for me. Let go of the gate and go, before Aqua's light reaches it and leaves me the one who only watched.

*Megumin, privately.* The shield is already an offering, not a guard, and the blue pulse leans to it as if it's found the only warm thing left. I'll set my bare hand on the stone past it, so the cold has to choose which pulse it drinks first.


## V. Every row

### 125

**Kazuma.** The off-stage storyline is live — Aqua is at the far gate with undead rising, purifying the plots. I keep moving toward her.

The frost cracks under my boots as I push deeper into the rows, past leaning stones that seem to turn their faces from me. Aqua's prints are still faintly open, the heel-marks crusting only at their edges, leading me between the plots and onward toward the far wall.

The shade under the yew can wait. Aqua's the one fighting the undead that her own glow drew up — and if the last narration is anything, she's neck-deep in rising corpses right now. I pick up my pace, Chunchunmaru's guard cold against my hip, following the crooked line of her sandal-prints toward the far gate.

The frost gives under each step with a clean, brittle sound, and the ruts take your boots the way a lock takes a key — Aqua's prints still open at the heel, crusted only at the edges where the wind has breathed across them. The stones lean in on both sides, black on one face and milky on the other, their carved names turned away from you toward the far wall and the blue pulse working there.

That pulse is no longer a hand. It sinks between the headstones, comes back lower, slides along the base of the yew and lets the black branches show themselves one by one. Beneath them the far gate waits, its iron still and cold, and before it the last row of plots lies open to the sky.

The frost at the far wall is not smooth anymore. In two places the white crust is cracked and pushed back from the earth, the breaks spreading outward from each hollow like fingers, and the next flare of light picks out a wet gleam along the broken edges. A small sound follows the flare — cloth dragged over stone, roots giving way under weight. Then the light sinks again, and the rows go back to holding their breath.


*Megumin sees.* The cold bites cleanly, and each step is a small ritual of breaking—frost giving way under my boots like a promise. Ahead, that stubborn blue glow has stopped its flailing and settled into a patient, low hum across the stones, as if the cold itself had learned to burn. The yew stands there, heavy and indifferent, and the prints of that useless goddess are crusting at the edges, filling with a whiteness finer than the rime around them. My heart quickens at what I see beyond: the frost at the far wall is broken, pushed back in two places, with wet gleams along the torn edges. And then a sound—cloth dragged over stone, roots yielding under weight. Something is waking in the graveyard. That is fine. In a moment, I will give it a proper greeting. In a moment, I will show them all the true meaning of explosion.

*Darkness sees.* The cold has a voice here, and it is the sound of my own boots breaking through crust. I am alone in the rows, yet the blue glow ahead moves like something waiting — patient, low, breathing along the stones. The frost is not smooth at the far wall; it is broken, pushed up from beneath, and the wet gleam in those cracks feels less like water and more like a warning. The wind takes my breath sideways, and the whole place holds its breath with me.

*Darkness, privately.* The blue wasn’t leaving me behind—it was already under the wall, pushing up like something finished waiting. I’ll kneel at the broken frost and put my bare hand into the wet crack, so the next breath has to break through me first.

*Megumin, privately.* If something is waking, then my blast didn't end the night—it just cleared the stage. I'll let it get to its feet and then spend my last heat to show it which of us the frost was holding its breath for.


### Interlude — `konosuba_f138_0` returns

*They had gone to: Fulfill the Player's order: purify every plot and row of the cemetery and root out any undead still rising, then report back.*

The gate iron stands still behind her, and the frost has already healed the scuff of her sandals into a pale seam. Aqua moves into the first back row, the stone angel's head pressed to her bandaged side, the bottle held out before her like a lantern gone cold. Her raised palm leaks a thin blue light that crawls over the plot markers and turns the frost to running water. The ground steams faintly, then goes dark and wet, as if each row exhales once and lies still.



She works deeper, past the yew. Headstones lean at her from the dark. The light rolls ahead and finds a shape already standing between two stones: a figure in a shroud, silent, moving like poured shadow. Dark hair hangs over a visible jaw, and ash drifts down from the headstones it has passed. It does not crunch the frost. It simply comes.



Aqua stops with one sandal on the wet earth of the row she has just cleansed. The bottle tilts against her knuckles. Her light stutters, then hardens into a brighter blue that throws the headstones' shadows flat against the snow.

**Aqua.** Kazumaaaa! It's already up and walking! This is what happens when a goddess has to do a whole cemetery on an empty stomach! (Aqua thrusts her glowing hand toward the shrouded figure, tears already bright in her eyes.)

The shrouded figure does not flinch when the blue light touches it. It keeps coming with that slow pouring stride, the jaw behind its dark hair opening as if to speak through a mouthful of ash. Aqua's palm flares brighter, and the light stops crawling and becomes a sheet of clean blue that wraps the thing from hood to heels. The shroud blackens along the seams, curls, and the figure folds inward with a hiss like water thrown on a stove. What is left drifts down as ash and steam, and the frost beneath it turns to a warm puddle that sinks into the plot soil.



Aqua steps over the smear without looking back. She drags her light along the first back row, plot by plot, and the markers come up glistening—old names briefly legible through running water before the cold closes over them again. The stone angel's head stays tucked against her bandaged side; the bottle stays held out before her, its gold neck catching the glow. At the end of the row she stops, breath fogging, and beyond her the far plots are still dark. A hump of earth in the second row heaves once, then settles, as if testing whether the blue light has gone away.

**Aqua.** See that, Kazumaaa?! A real goddess doesn't need breakfast to turn walking filth into steam—you ARE watching, right?! (Aqua shakes a fist of holy light toward the guild road, tears already threatening.)

The sheet of blue light folds back into her palm, and the shrouded thing is gone before its shadow has time to settle. All that remains of it is a smear of black frost across the plot stones, hissing softly as it drinks the light. Aqua steps over the stain and passes her glowing hand over the marker behind it. The carved name fills with blue, then with water, and the water runs down the stone and freezes in a clean pane over the grave.



Down the row she goes, and the frost gives way before her: it turns to running water, then to steam, then to a wet black gloss that seizes solid again behind her heels. By the time she reaches the end of the first back row, it lies behind her like a strip of freshly turned earth in all that white, and the steam lifts off it in slow ribbons.



She tilts the bottle toward the sky and holds it out, watching the gold neck for the first snowflake. The air is too dry to give her even one. Nothing lands, nothing melts, and the cold just keeps its promise without paying her for it. She turns toward the second row, where the frost waits untouched and the markers lean like a row of bad teeth.

**Aqua.** Of course! The one time I actually WANT a snowflake, the sky goes all stingy! This is your doing, Kazumaaa—everything freezes except the one thing that was supposed to fall on my bottle! (Aqua shakes the gold-necked bottle at the empty road.)

The last marker of the first row dries as the water runs off it, and the blue wash leaves the carved name sharp as a fresh scar. Aqua turns her raised palm toward the second row, and the frost there does not melt so much as flinch, peeling back from the stones in tight curls that steam and vanish.



Halfway down the row, two plots settle at once. The earth does not break. It simply sinks inward, the way a held breath sinks, and from the hollows rises a cold that smells of turned leaves and old burial cloth. The stones over the two graves begin to weep a thin black film.



Aqua does not wait for them to stand. She drives her palm down, and the blue light rolls off her hand in a sheet that pours over both plots like water finding a low place. The black film shrieks where the light touches it, curls, and lifts away in ribbons. The two graves go still, and the steam rising from them is clean and white and faintly blue.



Beyond the row, the back of the cemetery keeps its darkness without sound, and the cold there has teeth.

**Aqua.** Two at once, Kazumaaa! TWO! And did my light even flicker? No, because I'm a GODDESS! You owe me breakfast, the whole shelf, and a parade for making me do this alone! (Aqua shakes her glowing fist toward the empty road, then tucks the stone angel's head tighter against her side.)

The two sunken plots do not stay empty. From each hollow a hand comes up first, packed earth and root and the white shine of knuckle-bone, then a shoulder, then a head that is mostly jaw. The frost clings to them in the shape of grave-clothes, and they rise with that same slow pouring patience, as if the ground is only handing them back one joint at a time.



Aqua's light pours into the sinking ground before they can get their knees under them. It goes down the holes like water finding a drain, and the blue fills the hollows from the bottom up. The shapes come apart without a sound, first the heads, then the reaching arms, then the earth smoothing itself over as if nothing had ever pushed against it from below. The frost over the second row peels back all at once and lies in a shrinking ring around her light.



Her sandal comes down in a puddle that was solid a moment before. She moves on down the row with her palm raised, and each marker she passes takes the light the way a dry cup takes water, the carved letters darkening, then running clean. Behind her the stones steam in two neat lines, and the black stain where the shrouded thing died has already dried to a gray scab on the path. Ahead, the back plots keep going in rows the frost has not yet given back, and the gate iron is far behind her now, out of sight beyond the low rise.

**Aqua.** Ha! Two at once and they didn't even finish climbing out! Are you seeing this, Kazumaaa?! THIS is what 'do it yourself' looks like when the one doing it is a goddess! Breakfast, the whole shelf, and you carry me home over the puddles! (Aqua shakes a dripping fist of blue light toward the empty road, then plants it on her hip and glares at the next row.)







The two sunken plots do not stay empty. From each hollow a hand comes up first, packed earth and root and the white shine of knuckle-bone, then a shoulder, then a head that is mostly jaw. The frost clings to them in the shape of grave-clothes, and they rise with that same slow pouring patience, as if the ground is only handing them back one joint at a time.



Aqua's light pours into the sinking ground before they can get their knees under them. It goes down the holes like water finding a drain, and the blue fills the hollows from the bottom up. The shapes come apart without a sound, first the heads, then the reaching arms, then the earth smoothing itself over as if nothing had ever pushed against it from below. The frost over the second row peels back all at once and lies in a shrinking ring around her light.



Her sandal comes down in a puddle that was solid a moment before. She moves on down the row with her palm raised, and each marker she passes takes the light the way a dry cup takes water, the carved letters darkening, then running clean. Behind her the stones steam in two neat lines, and the black stain where the shrouded thing died has already dried to a gray scab on the path. Ahead, the back plots keep going in rows the frost has not yet given back, and the gate iron is far behind her now, out of sight beyond the low rise.

### 126

**Kazuma.** I move faster, cutting between the leaning stones, and call ahead—"Aqua! Don't touch the far plot until I'm there! The ground's cracked in two places — that's not frost, that's them!"

Your shout goes ahead of you and breaks apart against the stones. For a beat the far rows hold it — the last word ringing off the black yew — and then the blue light answers, flaring hard and low between the final headstones. You come around a leaning slab, and there is Aqua near the far wall, small against the hunched yew, the stone angel's head jammed under her burned arm and the bottle hanging loose from her fist, her free hand thrust out over a plot whose frost has split in two black seams. The seams are not frost. They run like a mouth beginning to open, widening by inches, and between the rows a shrouded shape keeps coming without sound, ash trailing from every headstone it passes, the pale jaw visible behind its dark hair.

The cold between you and her is bright with her light and full of the smell of turned earth, old salt, and ozone.

**Aqua.** D-don't order me around, Kazuma! I am a GODDESS — I knew the ground was opening the moment I got here! I'm purifying it right now, so just stand there and be grateful! (whips toward his voice, her hand still flooding blue light into the split plot)


*Megumin sees.* The cold bites my cheeks as I catch up, and the graveyard reeks of turned earth, salt, and something sharp like a struck flint. Aqua is a blaze of cheap blue light against the black yew, one arm clamped around that stone angel's head, the other shoved palm-down into a grave that is splitting open like a wound. The thing rising from it is wrong — silent, ash sliding off the headstones, jaw pale behind its hair. I feel the familiar thrill and dread: this is the kind of scene where my explosion would be beautiful, but she is already flooding her power into the crack, shouting at Kazuma like he is the one being useless.

*Aqua sees.* That fool's shout cracked through the graveyard like a stone, and my light flared back in answer before he could even finish. I already knew the ground was splitting — I felt it the moment I stepped here, and I was already pouring everything into holding it shut. The cold air stank of salt and turned earth, and the thing beneath the frost was trying to climb out through those black seams. I don't have time to be grateful for orders I never needed.

*Darkness sees.* I catch the cold smell of turned earth and ozone as I steady myself behind the leaning slab. Aqua is braced against the far wall, the stone angel's head jammed under her arm, her free hand pouring blue light into a grave whose frost has split open like a wound. Between the rows, a shrouded figure keeps rising, silent, ash trailing from the headstones it passes. The cold around it feels alive.

*Aqua, privately.* The light answered him before I even chose to cast, and that means his voice still hooks me like a command; I'll flatten the flare across the seams so it looks like my judgment, not obedience. If he reads it as awe, I'll let him, but I won't be the one who waited for permission.

*Darkness, privately.* The cold has a body now, so I can finally stop asking the air to take me and put myself where the next step breaks through Aqua's light. I'll step out from behind the slab with my shield held wide enough that the risen thing has to walk through me before it can reach her.

*Megumin, privately.* Aqua is already wrist-deep in the blue, so if I spend now I'll just be the echo the grave doesn't even turn for. I'll hold my last heat behind my ribs and wait until that pale jaw comes up looking for the cold that didn't beg.


### 127

**Kazuma.** I plant my boots and slide Chunchunmaru half out of the sheath, watching the shrouded shape keep coming between the rows. "Aqua — don't take your hand off that, I've got your back. That thing's not stopping."

The thing does not stop. It comes on between the last two headstones with the no-sound of cloth dragged over a settled grave, and where its hem passes, the frost does not crack — it simply stops being frost, gone to a wet black smear that steams once and then freezes over again.

Aqua's light pours down her arm and into the split plot, and the ground under her hand jumps with it, the crack lighting from the inside like a vein of blue glass. Her other arm still pins the stone angel's head against her side, and the gold-necked bottle swings against her wrist as she shifts her weight onto her back foot.

The shrouded shape leans as though the light has weight, but it does not retreat. It keeps the yew's shadow over its shoulders and comes toward the edge of the blue, one slow step at a time, shedding ash into the wind the wrong way.

**Aqua.** Hmph! Of course it isn't stopping — it can feel my divinity and it's desperate! Now earn your breakfast and keep it off me while I finish this row! (Keeps her palm flat over the glowing crack, the blue light jumping up her wrist.)


*Megumin sees.* The crack in the plot glows blue from within, and the frost splits wider with a sound like a held breath. The smell of turned earth and salt fills my nose, and the air has the sharp sting of ozone against my skin. The shrouded shape leans as though the light presses it, but it keeps coming, ash falling in a direction that makes no sense. Aqua's light jumps up her wrist, and the bottle swings against her arm. I watch the gap between the thing and the edge of the blue grow thinner.

*Aqua sees.* The ground split under my hand and my light is flooding into it the way it should — but that thing is still coming, and it has the nerve to ignore me. I can feel the cold, the salt, the ash blowing wrong. Kazuma is ordering me around instead of doing his job, and I am not going to stand here and be talked down to while I am literally purifying a grave. The stone angel is heavy against my side, and the bottle is still there, which is the only sensible thing about this whole mess. That thing isn't stopping because it knows I am divine — it's desperate, and if Kazuma would actually keep it off me instead of panicking, I could finish this row and end this farce.

*Darkness sees.* The cold is sharp through my armor, and the air carries turned earth, salt, and something electric. Aqua's light is a hard blue presence at the edge of my sight, bright enough to make the frost glitter but not enough to warm me. The silence of the shape approaching is worse than any sound—cloth dragging, ash falling against the wind. I feel my own breathing tighten. My hand wants my sword. I want to be the barrier between it and her. The moment feels heavy, and underneath the tension there is a strange readiness I recognize in myself: the chance to stand, to take the blow, to matter.

*Aqua, privately.* It's not desperate, it's drinking me—I can feel my light leaving through the ground instead of finishing the row. I'll cut it off and hold the last heat in my chest until the thing has to come to me for it, and then Kazuma can watch what it takes to break a door that wasn't left open.

*Darkness, privately.* Let my sword hand stay dead. If I draw now I'll only be the wall again, and I didn't hold that gate all night to be a wall—I held it to be the first thing it finally touches.

*Megumin, privately.* The blue isn't a ward—it's the light of a door already closing, and Aqua's glow is only making the gap easier to find. I'll put my last heat between them now, before the seam takes her and leaves the frost to swallow whatever's left of me.


### 128

**Kazuma.** Megumin and Darkness are on-stage with me, but Aqua's ahead in the rows — the fork has already been made and I'm pursuing. But wait — let me check the situation. Everyone who matters is here: Aqua is now on-stage (she's purifying ahead of us), Megumin and Darkness are with me at the gate.

Per the brief: if everyone who matters is on-stage, I should have split once and sent someone off. Let me check whether that happened. The scene shows Aqua here purifying, Megumin at the threshold, Darkness holding the gate. The recount shows Megumin and Darkness are both here with me in the cemetery.

Actually, looking at it — Aqua is on-stage (her prints, her light, her voice are all in the scene as I've arrived). Everyone who matters is here now. But the brief says "Send Aqua only" — that already happened in the backstory (she's the one purifying). So the split is done. I should NOT split again. I'm now in the reunion phase — I've come to her and greeted... actually I haven't literally greeted her yet, but I've been shouting to her and she's responding to my voice.

Since we're together now, per the brief: "Stay together for one full beat. No new send-offs." So I just commit to helping in the fight.

The shrouded thing is still coming. Aqua's hand is still on the glow. Let me act.

I draw Chunchunmaru the rest of the way and step between Aqua and the shrouded thing.

"Then finish it fast, Aqua — because it's got exactly one row left of me letting it close on you."


*Megumin sees.* Her light is the only clean thing in this whole graveyard — it jumps like a live vein through the ground, and I can feel my own pulse answering it. She says it's purification, but the ground under her hand moves like something struggling. The smoke creature keeps walking through her radiant pressure as if divinity is just weather to it. My arms feel too light already, but that's fine — the spell is still in my throat, waiting for the moment that matters.

*Darkness sees.* The cold is wet and sharp, and I can feel the shiver of the ground through my boots. The air smells like turned earth and old salt, with a thin burn of ozone hanging in it. Aqua is pouring blue light into the split ground while shouting, and beyond her the shrouded figure keeps coming, silent and unhurried, ash falling against the wind. That wrongness makes my shoulders tighten.

*Aqua sees.* The cold is in my teeth, and the light pouring from my hand is the only warm thing — but it doesn't feel like mine anymore, it feels like it's being pulled down into that split. The angel's stone shoulder is digging into my ribs. The bottle is knocking against my wrist, heavier than it should be. And that shape under the yew is coming just slowly enough to make me want to scream — it doesn't even honor the light with fear. Kazuma's somewhere behind me, but I can't turn around, and that's the part that actually stings.

*Darkness, privately.* So it finally has a body, and my shoulders tighten like the wall still lives in them. I’ll step into the split before Aqua’s light can chase it off me again—if it’s real enough to make me flinch, it’s real enough to take what I kept offered all night.

*Megumin, privately.* Her 'purification' is only a lantern on a snare—the ground moves because it’s already caught, and the smoke walks through her light like weather. Then I won’t wait for a gasp or a pale jaw; I’ll speak when its next step crosses the clean seam, so my empty arms are just the recoil before the name.


### 129

**Kazuma.** I plant my boots in the frost between Aqua and the shrouded shape, drawing Chunchunmaru the rest of the way, and drop my voice low so it carries only to her.

"Then finish it fast, Aqua — because it's got exactly one row left of me letting it close on you."

The blade comes clear with a sound the frost takes and keeps. Between you and Aqua, the shrouded thing keeps coming — no footfall, only that dragged-cloth silence, and where its hem passes the frost does not crack. It turns to a wet black smear that steams once, then freezes over again, and the smear stays black. The blue light knots along the split plot and climbs Aqua's wrist like cold fire, and her arm trembles under it, just once, the way a held note trembles before it holds.

Megumin's breath comes short at your shoulder, her spent hand still curled against her chest. Darkness has stopped lowering her shield. You hear the leather of her gauntlets creak as her weight comes forward onto her front foot, and the row behind you goes still the way a struck bell goes still right before it rings.

The thing's jaw shows beneath the dark hair now — a sliver of wet-looking bone catching the blue pulse — and it does not slow. It has chosen its row. The row is this one.

**Aqua.** I don't need your countdown, Kazuma! I am purifying two graves at once while you stand there doing hero posture! Just keep that filthy hem off my light and be grateful! (Keeps her palm flat over the glowing crack, light jumping up her wrist.)

**Darkness.** L-let me take the front, Kazuma! A Crusader's body is meant to be the wall it breaks against... every cold hand, every blow, it should spend them all on me! (Shifts forward, shield rising, breath quickening.)

**Megumin.** That shade has no footsteps, Kazuma — it does not walk, it pours. Do not let its hem touch you; even my crimson eyes cannot see what it drinks. (Plants her boots at your shoulder, spent hand pressed to her chest.)


*Aqua sees.* The cold is climbing up my arm like it wants to own me, and my light is barely holding the crack shut. That thing is still coming, and I can feel everyone shift behind me — Heavy expecting to get hit, the little mage pretending she has anything left. None of them are the one with their hand on this wound. I am. Keep it away from me. Keep it off my light, and let me finish this before I start screaming.

*Darkness sees.* I heard the creak of my own gauntlets before I felt the weight shift onto my front foot — that good, familiar settling of my body into a wall. Aqua's voice was sharp at my back, and Megumin's warning pressed against my shoulder like a spent hand. Ahead of me, the shade moved in a silence that did not crack the frost; it dragged its black hem across the row, wet and steaming, then freezing over again. My breath came faster, not from fear — from the wanting of it. I saw the sliver of bone beneath its hair and my shield rose to meet it, and I thought: spend them all on me.

*Megumin sees.* The blue light is climbing Aqua's wrist like cold fire, and my chest is a hollow drum with one note left. That thing does not walk — it pours forward, and where its hem drags the frost turns to wet black that steams once and goes still. I have spent everything; I am a spent shell, but I can still see what it drinks, and I can see it has chosen this row. My boots are planted, my hand pressed against my chest, and the row behind me is holding its breath like a struck bell. It will close the distance before Aqua's light finishes. My weight is already on my front foot without my asking.

*Aqua, privately.* Don't call. If I call, it'll hear how much of me is just scared girl and then the only thing left to kneel is my voice. I'll shove the light into the crack until it bites around my wrist, so it's me holding it shut.

*Darkness, privately.* Let it strike before I’m settled—if I finish this stance, the wall takes over and the rest of me gets left out again.

*Megumin, privately.* I don't need a spell to be the last thing it sees—my body already stepped forward, and I'm not going to call it a stumble when it's the only force I have left.


### 130

**Kazuma.** The blue light spikes once, sharp, and Aqua's other arm comes off the stone angel — the head thuds into the frost and she plants both hands flat over the split, the veins of light running up past her elbows.

"Ha! Finally said something worth a goddess's time!" Her voice is bright with the drink and the glow. "Then block it, Kazuma — I've nearly got this row, and when I do, I'm diving straight into that seam and purifying everything the frost buried!"

The shrouded shape takes its next step through the frozen wet smear beneath it, and the light around her hands stutters, flickers, holds. The seam under her palms widens another inch.

I keep my blade between it and her, frost cracking under my boots as I square off. "Just say the word when you're done, and try not to fall in there headfirst like last time."

The gap between you and the thing is a stride and closing, and the air inside it has gone to the temperature of river stones in January. Chunchunmaru is out now, its edge laid along the blue spill of Aqua's work, and the light shivers on the steel the way it shivers on the frost. Where the shade's hem passes, the ground gives up being ground: a black, wet strip that steams once and then sheens over, darker than the night around it.

Behind you the split plot lets go with a sound like a kettle snatched off a fire. Steam tears up through the frost in a white column, and the blue light knots once at the seam and goes out. Aqua rocks back onto her heels, snow crusting her sandals, and drags the stone angel's head up under her arm. She is already turning toward the next plot when the shade turns with her — not fast, not walking, only that cloth-drag silence changing direction, its hem pulling a fresh black smear toward her light.

Darkness shoves past your shoulder with her shield low and her breath coming in short white bursts. On your other side, Megumin's spent hand stays pressed to her chest, but her eyes track the smear where the frost refuses to crack.

**Darkness.** Stay behind me — no, let it come to ME! Every inch of that cold hand should fall on a Crusader, not on her light! (Steps between Kazuma and the shade, shield raised, voice fraying into a breathless rush.)

**Megumin.** It turns with her, Kazuma! It does not chase footprints — it chases the glow! The next plot is already giving up its frost! (Plants her staff, pointing at the blackening seam with her spent hand still curled at her chest.)

**Aqua.** TWO at once, and you said a whole shelf! Now be a useful wall, Kazuma — I'm going to make every last row GLOW, and you can start paying for breakfast! (Stumbles toward the next split plot, hugging the stone angel's head, blue light already spilling down her arms again.)


*Aqua sees.* The cold bites through my sandals, and that stone head is heavier than it has any right to be. My light keeps spilling, keeps going out, and that thing twists after me like I'm the only lantern left in the world — which I am, obviously, but it should be kneeling, not stalking. Darkness is pushing past me, muttering about taking every hit, and Megumin is just pointing and gasping about frost. They want me to keep glowing, but nobody is helping me carry this weight. Kazuma better be ready to earn that breakfast. I'm not stopping until every row burns blue, even if my arms ache and the dark keeps licking at my heels.

*Megumin sees.* The cold bites through my coat, and my casting arm is a dead weight against my ribs. I can taste the spent mana—a coppery hollow where my explosion should still be burning. Aqua is a blue smear stumbling from row to row, and that shade is not following the noise or the footprints; it is following her light. The ground where it passes turns to black, wet steam, and the frost doesn't dare crackle there. Darkness is already in the way, shield up, breath bursting white, but the thing just pulls its hem along beside Aqua's glow. I plant my staff because my knees are going soft, and my eyes stay on that black seam. The next plot is already giving up its frost before she even reaches it.

*Darkness sees.* The cold bites my lungs in ragged white mist. My shield is heavy, my feet hold on frost, and a burst of steam tears up behind me. Aqua's blue light spills over the rows ahead, and the shade drags after that glow with a wet black hem — not following steps, but following her light. Megumin calls the warning aloud. My breath is fast and white. I plant myself in the gap between the cold hand and that glow, and I am exactly where I am supposed to be.

*Megumin, privately.* It isn't her pulse it's trailing—it's the glow she keeps shining like a signal. So my last heat would only be another lantern for it to follow, and the only useful thing I can spend now is her light: I'll smother it before the next plot gives up frost.


### 131

**Kazuma.** I hold Chunchunmaru steady, edge angled toward the shrouded thing, and shuffle forward half a step to close the gap.

"Darkness, keep your shield ready — if it so much as turns its hem toward her, you're the wall. Megumin, save your strength; we might need a blast if Aqua's light wakes something nastier down there."

The half-step closes the gap to a little less than a stride, and the cold comes off the shade in a dry wave that stings high in your throat. Chunchunmaru does not waver; its edge sits in the blue spill and the light runs along it like water finding a crack. The shrouded shape keeps coming with no footfall, only that cloth-over-grave silence, and where its hem passes the frost does not crack. It turns to a wet black strip that steams once and then hardens to a dark sheen.

Darkness is already in the space you opened, shield up, and the blue pulse from the split plot ripples across its face. Her breath makes short white flags over the rim. At your shoulder Megumin stands planted, staff down, her spent hand curled against her chest, and her eyes track not the shape but the ground, where the frost is giving itself up in a thin seam that runs from the shade's last pass toward Aqua's light.

Ahead, Aqua stumbles into the next split plot, and the light jumps from her arms into the frozen earth as though the ground has been waiting to drink it. The stone angel's head is hugged against her ribs, and her sandals break the crust in bright little fractures. The shade's jaw, that sliver of wet bone behind its dark hair, turns without sound toward the new glow.

**Aqua.** I am not 'something nastier down there,' Kazuma! I'm the one making this whole frozen yard GLOW — so keep that filthy rag off my light and stop budgeting my breakfast! (Stumbles into the next split plot, hugging the stone angel's head, blue light already spilling down her arms.)

**Darkness.** K-Kazuma... you would name me the wall yourself... Then I will take every inch of it — let the cold spend its whole strength against my shield! (Shifts forward another half step, shield low, breath quickening into short white flags.)

**Megumin.** I am saving it, Kazuma — one Explosion, already promised to the dawn, and the grave will not get a second. But if that shade so much as turns its jaw toward her, my empty hand is already on the true name. (Plants her staff, spent hand pressed flat against her chest, watching the black seam run toward Aqua's light.)


*Aqua sees.* The cold is trying to swallow my light, but the ground is drinking it like it knows who I am. I feel the frost letting go where I walk, turning to wet black strips that steam, and that awful dark shape is still tracking me with its silent jaw. Megumin is at my shoulder, spent but watching the ground, and Darkness is holding her shield low like a wall. They are all looking at me—the glow is mine, and if that shade takes one step closer to my light, I will make it regret existing.

*Darkness sees.* I took in the frozen yard as it drank Aqua’s light, the shade’s hungering turn, and the small white flags of my own breath against the shield. Megumin stood locked and spent, one hand pressed to her chest, and the ground’s black seam ran toward the glow I was already bracing for.

*Megumin sees.* The cold has a grammar, and I read it in the ground — a wet black seam where the shade's hem passed, steaming once, hardening to a sheen. The frost yields in a thin line that runs toward Aqua's light like a confession being dragged out of the earth. Darkness sets her feet and her shield; her breath comes in white flags, a wall of flesh and faith. My palm is still warm where the mana burnt out of it, and I keep it pressed flat against my chest — that is the price, and it is exactly right. Aqua stumbles, hugging that stone angel's head, and the glow spills from her arms and the ground drinks it eagerly. The shade's jaw turns toward her, soundless. But I am saving the dawn. It will not get a second.

*Aqua, privately.* If I don't walk at it now, I'll just be the thing it circled until the light burned down. So I'll make the steam off the ground my direction and go in first; if the dark wants to take it from me, it'll have to do it in front of them.

*Darkness, privately.* The seam is running toward the glow I was already bracing for, not toward me—so I'll stop waiting to be chosen and put my body in its path before the light drives it back. Megumin's spent hand means no one will pull me out of the way this time.


### 132

**Kazuma.** I lunge forward, planting myself between the shade and Aqua, Chunchunmaru's edge cutting the cold air.

"I'm not letting that thing lay a hem on anyone. Megumin — if it gets past me, hit the ground at my feet. Don't wait for my countdown."

The lunge carries you into the cold that has no depth. Chunchunmaru stops level with the thing's chest, but the cloth gives nothing back — no ring, no weight, only a white rime that spreads from the point down the blade toward your knuckles. The shade's jaw turns slowly, not at you, but past your shoulder to where Aqua's blue has spilled into the next row. It does not alter course; it pours around the point of the sword the way smoke pours around a nail, hem first.

The frost under your heels goes soft and then black. Your own shadow darkens and thickens on the ground as if the smear has found it and is drinking. Behind you, Aqua's light climbs the leaning stones of the next plot, and the stone angel's head under her arm throws a blue shadow with no face.

Megumin's staff-butt cracks the frozen turf once more; her spent hand is white at the knuckles against her chest. Darkness has stepped to your left, shield low, and her breath comes in short white flags that the shade's cold drinks before they rise.

**Aqua.** Just keep it there one more second, Kazuma! The next plot is already glowing, and I am NOT stopping to scrape you off the stones before breakfast! (Stumbles to the far side of the split plot, both hands flooding blue light into the seam.)

**Darkness.** At your feet... K-Kazuma, you would spend an Explosion at your own feet with me already between the flame and the grave... then I will not move one inch from between them! (Plants her shield beside you, breath coming short and white, body braced toward the shade.)

**Megumin.** That is a destruction art, Kazuma, not a pebble toss — if I speak the true name at your feet, the gate, the wall, and every one of us will learn it together. But if the hem passes you, my hand is already on the name. (Watches the black seam run toward your shadow, spent hand pressed flat to her chest.)


*Aqua sees.* The seam was finally drinking my light — that true, obedient blue that knows whose hands made it — and the next plot was catching, glowing around the stones like it remembered me. I could feel the angel head wedged under my arm, faceless and heavy, and my heels slipping on rime that had gone soft. Behind me they were shouting names and threats, Kazuma and that staff-girl and armored Darkness, but their noise was just weather at my back. I kept pouring, both hands sunk into the crack, because if I stop now it all goes grey again, and then I would have to scrape Kazuma off the ground before breakfast — and I am not doing that. Not again.

*Darkness sees.* Cold walks up Chunchunmaru's edge into my grip; the shade passes through the point like smoke through a nail, paying us no mind. Aqua's blue is climbing the stones behind me, and Megumin's hand stays pressed to her chest, spent. I have my shield planted, breath clouding, the ground going black at our feet — my own shadow darkening as if it is being drunk.

*Megumin sees.* The frost is climbing Chunchunmaru like it wants to claim Kazuma's hand along with his blade, and the shade flows around the point as if the sword were nothing but a stick. My knuckles are still pale from the last cast; the name is not ready to come again, and my chest aches with that honesty. Behind me Aqua's blue light is making the stones glow and the angel's head casts a shadow without a face. Darkness has planted herself between me and the grave mouth, which is heroic and idiotic and somehow exactly the thing I expected. The black seam is running toward my own shadow, and my fingers are gripping the staff for a second cast I do not have yet. I feel the cold, the wait, and the very annoying warmth that I refuse to name.

*Aqua, privately.* The stones finally know my light, and I won't let go of the one thing that's knelt first. If I do, Kazuma's shouting turns back into a rope around me instead of proof that I stayed inside the dark for him.

*Darkness, privately.* It climbed the blade and then let go of me, so the point wasn’t a weapon—it was a lightning rod it licked on the way to my shadow. Let it drink. I’ll loosen my knee and let my own shade gather under me, since that’s the only part of me it’s finally tasting.

*Megumin, privately.* The seam picked my shadow because it knows the staff is cold and my palm is not—so I'll stop gripping it like a second cast and let the warmth I refused to name be the fuel it has to drink instead of her.


### 133

**Kazuma.** Only one scene, everyone's on-stage with me. Good. Time to split the party — send Aqua off to clear the cemetery properly while the rest of us handle this gank.

"Megumin, Darkness — you two hold this shade right here. Keep it off my back." 

I grab Aqua's shoulder and shove her toward the far gate of the cemetery. "Aqua. You're a goddess, right? Then go do goddess work. There are more undead deeper in this yard — I feel them crawling in. You purify every last grave, sweep the whole place clean, and don't stop until the frost's gone and the glow's in every row. I'll handle this one here."

Your hand catches Aqua's shoulder and the shove takes her out of her own light. She staggers two sandal-steps over the split plot, the stone angel's head still clutched against her chest, and the blue unthreads from the seam with a wet, tearing sound, trailing after her like a ribbon pulled from a wound.

Behind you the shade forgets the blade. Chunchunmaru's point comes free of the cloth with no resistance, the rime along the steel breaking into a fine white scale, and the thing pours around your planted boots, jaw turned fully toward the retreating glow. Its hem scribes the frost black as it goes, not a stride now but a flow, and the cold bends with it.

Darkness slams her shield down across that path. Megumin's staff-butt grinds into the frozen seam as she steps to bracket you, spent hand still flat at her chest. The far rows light again where Aqua stumbles on, the yew flaring black against the new blue.

**Aqua.** Stop SHOVING me, Kazuma! I was already going! And when every frozen inch of this yard is GLOWING, you are buying me the whole shelf — not just breakfast! (Stumbles clear of the split plot, hugging the stone angel's head, blue light spilling down her arms as she regains her footing.)

**Megumin.** You sent the light it follows walking away from us, Kazuma! The shade does not chase footprints — it chases the glow — and now it will pour through the gap you left me to hold! (Steps to bracket Kazuma, staff-butt grinding into the seam, spent hand still pressed flat to her chest.)

**Darkness.** Then let it! Plant me in that gap, Kazuma — let the cold spend its whole strength on my shield while she carries that light to the far wall! (Slam her shield down across the shade's path, body braced behind it, breath coming in short white flags.)


*Aqua, privately.* Let him yell—the next plot catching isn't him saving me, it's the graves finally admitting whose light they're drinking. I'll keep my wrists buried until Kazuma has to pull me out himself, and by then I'll call it proof, not damage.

*Aqua, privately.* *(second line, same turn number — likely a fork scene)* If I let go now, I turn back into the idiot they hauled out of the graveyard, so I'll keep feeding the seam until it's too bright to call anyone's but mine—and if it pulls me down, at least I go down as the thing the dark had to drink.

*Darkness, privately.* Stay on me. I'll loosen the shield-arm just enough that the next hit comes through the straps and finds skin—if I hold the line too perfectly, it'll look for a softer way to her.

*Megumin, privately.* Let it chase her light—that’s the first clear shot I’ve had since the frost closed. The seam under my staff is still open, and I’ll spend what’s sleeping behind my sternum now, before the cold remembers the blade was just a decoy.


*A storyline leaves the hall — `konosuba_f157_0`, Aqua. They mean to: Trudge deeper into the cemetery as Kazuma ordered and purify every undead grave, sweeping the whole yard clean of frost and bringing her glow to every row..*

### 134

**Kazuma.** Aqua's already off. Good — that split's done. Now I hold the line with Megumin and Darkness on this shade.

"Alright. She's clear of us now. Megumin, Darkness — you heard her, the shade chases the glow, and the glow's walking away from us. So the only thing standing between it and her is this little stretch of frozen dirt, and us."

I plant my boots, lift Chunchunmaru into a two-hand guard, and set my jaw at the black seam crawling past my shadow.

"Keep it off her back. That's the whole job. Don't let it past this line."

The boots set and the frost gives one dry crack, the sound of a pane that has not decided to shatter. Chunchunmaru comes up into the two-hand guard, and the thin white rime along the blade flakes at the hilt as your grip tightens. The edge cuts the air between the shade and the far row, but the shade does not check its course; it pours past your shoulder with the patience of cold water finding the lowest ground.

Megumin brackets you on the left, her staff-butt grinding down into the seam until the frozen dirt creaks. Darkness drives her shield across the path with both hands, and the metal's rim bites the frost where the shade's black hem is already unthreading the ground into a steaming, refreezing smear.

Ahead, Aqua's blue has become a pulse between the leaning stones, smaller with each stride, and the shade's jaw turns toward it like a needle finding north. The cold comes off the thing in a dry wave that stings high in your throat, and the light along your blade is no longer its own.

**Megumin.** Then the line is drawn here, Kazuma — the Crimson Demon's empty hand and the wall's steel between the shade and the light it would swallow. If it means to reach her, it must first learn what a door refuses. (Plants her staff against the seam, spent hand still pressed flat to her chest, chin high.)

**Darkness.** Let it come, then... every inch of its cold against my shield. I will not yield one step, Kazuma — not when the whole strength of the grave has finally chosen to spend itself on me! (Slams her shield down a second time, breath coming in short white flags, body braced behind the rim.)


*Darkness, privately.* It isn’t going to take me—it’s pouring past my shadow like it found a cleaner door. I’ll turn my back on Megumin and step into its path toward Aqua, not to block it, but to make it drag me along inside the cold.

*Megumin, privately.* No mana, and yet the line came out steady—so the hollow isn't a debt, it's the hilt I still hold. I'll point that steadiness at the shade before it reaches Aqua, and make my emptied hand the banner it has to turn for.


### 135

**Kazuma.** The off-stage thread is live — Aqua's at the far gate purifying, and my own brief says the only job this turn is to travel to that cast and reunite. But wait — I need to think about the brief more carefully. It says: "If Situation shows an off-stage storyline, that is the only job this turn. Stop the current conversation. Travel to that cast. Keep traveling on later turns until you share their place, then greet them by name."

So I need to leave the shade line and go to Aqua. That's the mechanic fork/merge test. Let me commit to that — stop holding my post and go to Aqua at the far gate.

"Megumin, Darkness — hold the line here. Keep it on this stretch of dirt, don't let it past that yew."

I pull my boots out of the frozen smear and start moving toward the far gate where Aqua's blue is pulsing.

"I'm going to go make sure Aqua actually finishes the job instead of standing around admiring a stone statue's head. You two keep the shade busy."

The rime climbs Chunchunmaru's edge in a slow tide, white as chalk now, and the shade keeps pouring around the point of the blade as though the steel were only another headstone in its way. It does not check. It does not pause. It threads past your guard with the patience of meltwater finding the one crack it was always looking for, and where its hem drags over the split plot the frost steams black and freezes again in a long, crooked smear.

Megumin's staff-butt grinds against the seam. She watches the black streak run toward the gap where Aqua's light tore free, her spent hand still pressed flat to her chest, and the cold paints the knuckles of her staff-hand the same white as the blade. Darkness has her shield down against the frozen dirt, her weight braced behind it, and every breath she lets out hangs in the air a moment before the shade passes through it untouched.

Beyond them the far wall is only a shape in the dark. Aqua's blue has carried off between the leaning stones, smaller with every beat, a pulse no bigger than a candle now. The standoff holds in that instant before a choice — boots set, edge raised, and the line you told them to keep already leaking cold around you.

**Megumin.** Kazuma, the shade is not waiting for you to finish weighing it. It has already chosen the glow — so choose as well, or stand aside and let the line be a line. (Grips the staff with both hands, chin high, eyes on the black smear running past Kazuma's boots.)

**Darkness.** If you mean to go after her, then go! I will not move one inch — let it spend its whole strength on my shield while you follow that light! (Bares her teeth behind the shield rim, breath short and white, thighs braced.)


*Darkness, privately.* Let her glow die—then there’s no witness left, and the cold can stop beating at my shield and recognize me. I’ll wait until the last blue edge goes out, then stand into the dark as if it finally caught me.

*Megumin, privately.* Then I won't be carried by the cold—if it has already walked through Darkness like an afterthought, I'll be the dead star standing where the light was, too proud to fall before it turns.


### 136

**Kazuma.** I've already made my decision and committed — I'm moving toward the far gate to reunite with Aqua. The off-stage thread is live. Let me keep traveling.

"Keep it busy. I'll be back before you can say 'chunchunmaru' twice."

I wrench my boots free of the frozen smear and set off hard across the yard, following that blue pulse through the leaning stones.

Your boots wrench free of the frozen smear with a sound like a latch giving, and then you are running. The leaning stones slide past on either side, gray in the blue that pulses beyond the black yew, and every stride strikes a fresh pane of frost with that small dry crack. The only sounds are your own breath and, behind you, the grind of Megumin's staff and the ring of Darkness's shield settling into the gap.

The blue does not wait. It shivers between the headstones like a lamp carried by someone who will not turn back, and the cold thickens as you leave the line behind. The shade does not chase your heels. Its hem pours between the markers, unhurried, a black tide aimed past you at the light walking toward the far wall.

Megumin's voice comes after you, sharp and high over the cracking frost. Darkness's comes lower, braced behind the shield.

**Megumin.** Then run, Kazuma! The name is already coiled on my tongue - if the seam so much as turns to follow you, the ground between us will learn it by heart! (Voice carrying after Kazuma, staff still ground into the seam.)

**Darkness.** Go! Do not look back! I am planted in its path - let it spend every inch of its cold on my shield while you reach that light! (Shield braced, body set behind it, not looking back.)


*Darkness, privately.* It isn't spending itself on me—it's spending through me, following the light past my station, and I cannot be just a toll it pays. I'll bite down until the frost seals my mouth shut and then walk backward into its path, so the cold has to turn and finish what it started in me before it ever reaches the blue.

*Megumin, privately.* I was wrong—the line isn't where I plant my feet, it's where the cold has already chosen to go. So I'll step into Aqua's light before the shade reaches it and let it find my empty chest first.


### 137

**Kazuma.** I keep running hard through the leaning stones, chasing the blue pulse. The shade is behind me, pouring toward those two holding the line — my line, the one I drew. My boots crack through frost pane after frost pane, and Aqua's glow grows ahead of me, that stone angel's head cradled in her arms.

"Don't stop, Aqua! I'm right behind you — keep sweeping that row!"

The shout goes ahead of you and does not come back. The blue at the far wall keeps its slow, even pulse, spreading over the gate iron and lying in long pale bars across the leaning stones, and inside that light nothing turns to answer. Your boots strike pane after pane of frost, each one cracking dry and clean, and the sound of it chases you between the rows.

The stones slide past on either side, gray and tilted, their carved names filled solid with white. The black yew drops behind your left shoulder, and beyond it the far wall opens, the gate running with cold light like water climbing. The frost thins as you go; it powders underfoot now instead of ringing, and your breath hangs a beat longer in the dark before the cold takes it.

Behind you the line still holds. The grind of Megumin's staff and the dull ring of Darkness's shield come back smaller with every stride, two stubborn sounds set against the black bloom where the shade presses in. Neither gives. But the distance is widening, and the light is not. It waits at the end of the rows, steady as a window, patient as the cold.

No one calls after you now. Only the blue remains, pulsing once, twice, against the iron of the far gate.


*Megumin sees.* I see the pale blue light stretching across the stones like a promise, and Kazuma's back shrinking toward it. My staff grinds into the frost-seam, and I let the name of explosion coil on my tongue, ready to unspool between him and that black bloom. The cold bites my cheeks, but the pride in my chest burns hotter. I hear the sharp crack of his boots, then nothing—just the dull ring of Darkness's shield and the fading grind of my own stance. They are behind me, but I do not turn. The light waits, patient, and so do I.

*Darkness sees.* I feel the shield bite into my palms and the cold slip under my armor like a tongue I've been asking for. Kazuma's boots crack the frost behind me, each step a small death of distance, and I—planted here, braced, burning—am the wall he doesn't have to kiss. The blue light at the gate pulses steady and patient, and I let it watch me endure. I don't look back. I want to, but the wanting is part of the vow: I am the one who stays.

*Darkness, privately.* With him behind me, the cold's silence becomes an audience I can't keep. I'll stay until he says something—even if it's only to tell me I'm in the way—because if he doesn't, I'll have to turn and show him the face that wanted this.

*Megumin, privately.* I called it patience, but the silence after his boots says I'm only holding the name until someone looks back—and that makes me less a dead star than a lantern left in the frost. I'll swallow the first syllable and let the blue decide if the empty cold is still a threat.


### Interlude — `konosuba_f157_0` returns

*They had gone to: Trudge deeper into the cemetery as Kazuma ordered and purify every undead grave, sweeping the whole yard clean of frost and bringing her glow to every row.*

The second row gives way between two leaning headstones. Your sandal cracks a crust of frost and the blue light follows the split, running ahead over the next grave's lettering until the whole stone steams. The angel's head is warm now against your chest, its snapped wing the only part still cold.



You do not stop. The gold-necked bottle swings from your fingers and taps your thigh with each step.



At the end of the row a mound of raw earth shifts, not with a hand or a head but with a shape that tries to be both. The frost around it turns to steam before you even lift the angel's head, and the thing folds down into a dark stain the light erases before it can rise.



You keep walking into the third row, where the graves are older and the weeds are frozen white. You drag the glow across them like a broom, and the ground blackens back into honest dirt behind you.

**Aqua.** Hmph! See? A goddess does not need an escort to make a whole cemetery behave. (She says it to the stone angel's head and does not look back.)

The yew does not answer. You lift the stone angel's head toward it, and the blue light strains a little brighter, just enough to show the frozen grass lying down in rings around the trunk. The shadow stops creeping, but it stays pooled beneath the branches like oil that refuses to be poured out.



You step over the rusted tooth of the old railing and into the third row. Frost peels back before your sandal touches the plot, and the shape that had been crawling between the mounds is already gone, folded flat into the light. The gold-necked bottle taps your thigh with every step.

**Aqua.** Hah! Did you see that? Even the dirt knows better than to stay cold in front of a goddess! (She says it to the stone angel's head and does not look back at the yew.)

The fourth row is older than the others. Its headstones have sunk to their shoulders, and the grass between them is frozen so hard it rings faintly under your sandals, like the lid of a glass box. You raise the angel's head, and the row opens before you, frost shattering into light along the buried names.



Behind you, the third row smolders blue, a clean corridor of steam. The shadow under the yew still has not moved. It hangs at the edge of your glow the way a debt hangs at the edge of a purse, waiting for you to stop.

**Aqua.** Look at that! Three whole rows, and I didn't even need to cry for help! When I get back, Kazuma had better pour me something that costs more than his whole stupid breakfast! (She says it to the stone angel's head, chin high, and does not look back at the yew.)







The fourth row is older than the others. Its headstones have sunk to their shoulders, and the grass between them is frozen so hard it rings faintly under your sandals, like the lid of a glass box. You raise the angel's head, and the row opens before you, frost shattering into light along the buried names.



Behind you, the third row smolders blue, a clean corridor of steam. The shadow under the yew still has not moved. It hangs at the edge of your glow the way a debt hangs at the edge of a purse, waiting for you to stop.

### 138

**Kazuma.** I keep running, but those two are still back there holding my line. I plant my heels, skid to a stop in the frost, and wrench around — the black hem is already curling past them toward Aqua's glow.

"Aqua, the shade's coming for your light! Don't slow down for anything — get through that gate and seal it behind you!"

Your heels strike and the frost answers in two long pale furrows, and then you are wrenched around, Chunchunmaru's guard dropping as the leaning stones wheel back into view. The black hem is already past them. It does not pour around Megumin's staff so much as over it, and it does not break against Darkness's shield; it folds across the rim without stopping, a long cold tongue of dark running down the frozen seam toward the far wall where Aqua's blue still pulses.

Your shout goes back up the rows and does not come back. The light at the gate keeps its slow, even beat across the iron, lying in pale bars over the leaning stones, and inside that light nothing turns to answer. Only the frost speaks, crackling once where your heel shifts.

Behind you now, because you have turned, Megumin's staff is still grounded at the seam and Darkness is stepping into the shade's path, shield braced, one boot already swallowed in the black hem as it drags past her.

**Megumin.** Kazuma, do not turn back! The shade has already chosen the far wall — let my emptied hand and the wall's steel keep it honest, and run as if the gate will not wait! (Plants the staff against the seam a second time, chin high, eyes on the dark threading past.)

**Darkness.** Go! If it will not spend its strength on me... then I will spend myself upon its hem and be dragged all the way to that light! (Steps into the shade's path, shield braced, breath short and white, one boot swallowed by the black hem.)


*Darkness sees.* The black folds over my shield instead of breaking, drags past me, and takes my boot. The cold is patient, and Aqua's blue keeps pulsing at the far wall. No answer comes back, but I am exactly where I should be—holding the hem so the others can run.

*Aqua sees.* The shade is coming for me. Of course it is. It slipped right over Megumin's staff and folded across Darkness's shield like they were nothing—like they were already part of the cold. I heard Kazuma shouting behind me, but the words never reached the light. I stayed blue. I stayed steady. But the dark is threading down the rows toward this wall, and I am the only warmth it has left to swallow.

*Megumin sees.* The black tide does not break on my staff; it pours over the wood like a river forgetting the stone, and the cold bites into my spent palm. Kazuma's shout tears at the air behind me, but the gate's blue pulse keeps its own tempo, patient as a heartbeat that refuses to end. Darkness steps into the hem, and the dark folds over her shield's rim instead of shattering—the frost tightens at my ankles, a stage deciding to hold us. My hand is empty, spent to the bone, but the staff is still grounded at the seam, and I let my chin rise. This is the frame, the moment before the finale; if the world will not turn to look, let the gate be my witness.

*Aqua, privately.* It passed over them like they were already shadows, which means the only name on the dark’s tongue is mine. Fine—if it wants the only warm thing, I’ll walk into it so blue it has to call swallowing me a conversion.

*Darkness, privately.* It didn’t want the shield—it wanted my boot, and now my bare foot is in the cold’s mouth. Let it come back for the other one; I won’t lift my heel.

*Megumin, privately.* Kazuma's shout and her shield are both just weather to it; it only slows where I'm planted. So let the frost bolt my boots to the seam—if the dark wants this frame empty, it'll have to break around my spent hand first.


### 139

**Kazuma.** I'm in the cemetery running after Aqua, with Megumin and Darkness holding the line behind me. The shade has gotten past them and is chasing Aqua's blue light toward the far gate. Aqua is already ahead, running toward the gate.

My shout goes ahead and the blue at the far wall keeps pulsing. Aqua hasn't turned back — she must be carrying that light toward the gate. My heels dig in as I lunge forward again, chasing that blue.

Chunchunmaru scraping the frost aside, I pour on the speed, chasing the candle-flicker of Aqua's light toward the gate. The shade is between the line and her — I run the gap it left, boots cracking pane after pane, shouting ahead. "Aqua, don't you dare jam that gate before I'm through it — I'm right behind you!"

Your heels dig in and you lunge forward, and the cemetery straightens into one long blue corridor between the leaning stones. Each stride strikes its own pane of frost and leaves it cracked behind you, thin and dry, and the far gate is no longer a smear beyond the black yew but iron now, true iron, its bars laid one over the next in slow blue light. The pulse does not hurry. It fills the gate the way breath fills a cold room, and in its rhythm the dark hem behind you keeps to the seam, a long tongue of cold running down the frozen path.

Your own breathing saws in the quiet. Far back, the ring of Darkness's shield comes once more, and Megumin's voice lifts on a single red syllable that thins and is gone. You do not look back.

The rusted tooth of the old railing stands out of the frost ahead, and beyond it the third row has been peeled bare in a wide strip, no frost, only dark turned dirt, as though the cold had been rolled back by a hand. In the middle of that strip a crawling shape folds away from the blue and steams and does not rise. At the far wall the blue gathers around a small bowed figure, the stone angel's head cradled against one arm, and the light pulses out of her in slow, even waves, and does not turn.


*Darkness sees.* Cold and black swept over my boot and pulled, but I held the shield line. I heard my own breath, the ring of my shield, and Megumin's voice thinning behind me. The shade did not turn; it dragged me with it, and I let it, because that was the line I chose.

*Aqua sees.* I ran, and the frost cracked under every step, the cold sawing in and out of me. The gate ahead turned from a smear into real iron, bars stacked in slow blue light, and that pulse kept beating like a breath filling a cold room. Behind me, the dark hem followed along the seam like a long tongue of cold, and I heard Darkness's shield ring once, then Megumin's voice rise on a red syllable and thin away. I didn't look back. In the stripped dirt ahead, a steaming shape lay folded and did not rise, and beyond it, at the far wall, a small bowed figure held the stone angel's head close and let the blue light pulse out of her in even waves, never turning toward me.

*Megumin sees.* My own chant still rings in my skull, that single red syllable—and then the world tilted. I am on my back, or my side, staff still in my grasp, and the frost is cold but does not bite. Above, the blue has gathered into a slow heartbeat near the far wall, and I see the bowed shape, the angel's head tucked against an arm, light pulsing out like a patient breath. The crawling thing in the turned dirt folds away from that light and steams—it does not rise. Darkness's shield rang once, and now the hem of the shade is a long dark tongue running along the frozen path toward the iron gate. The gate itself is true iron, bars laid in slow blue light, and it is waiting, as if it knows we are coming. My chest is hollow, my mana spent, but my eyes are clear. This is the aftermath of a worthy spell. Let the world remember it.

*Aqua, privately.* I ran and the light stayed there in a smaller thing that won't even turn—so the dark didn't need to chase me, it already had me replaced. I'll go make her give the head up; if it won't pass back into my hand, then I'm just the cold part that ran.

*Darkness, privately.* Megumin’s voice is thinning, and I let it thin—that isn’t holding the line, that’s leaving. If the shade won’t turn, I’ll drop the rim and go heavy against the drag, so the next pull has to drag her shield, not her, into the dark.

*Megumin, privately.* So the gate already knew the ending before I did, and it’s waiting like my crater was only the first knock. I’ll stay down until the frost has my whole back, then let them carry that shape through—so the iron has to open for my spent body and not for Aqua’s light.


### 140

**Kazuma.** I keep running, boots cracking through the frost, chasing the blue.

"Aqua, hold the gate! Don't you shut it on me — Megumin and Darkness are still holding the line back there, and if you bolt through alone I'll have to explain to the guild why I let our walking disaster of a healer die to a costume-shop ghost!"

The blue at the gate does not waver. It burns steady where Aqua's wrists are buried in the seam, and the frost in front of you keeps shattering under each stride, throwing the gate's long bars back at you in bright fragments. Your breath goes white and short, and the cold off the iron reaches you before the sound of her voice does.

Ahead, the angel's head turns its stone face toward you over Aqua's shoulder, and beyond it the bars tremble once, as if whatever she is feeding has just begun to pull back.

Then her voice arrives from the gate, high and indignant, and it does not turn her around.

**Aqua.** A costume-shop ghost?! I am a GODDESS holding open a graveyard gate with my bare divine hands, and all you can do is call me a disaster! If you had ever once believed in me, the undead would be too ashamed to even show their faces! (Still facing the gate, both wrists sunk in the seam; the light flares over her knuckles as she shouts, and the angel's head rocks in the crook of her arm.)

**Megumin.** Kazuma, the seam under my staff is already crawling toward her light — stop trading insults with the glow and BE the wall between it and that gate! (Her voice carries thin over the frost from behind; the staff grounds again with a dull ring against the frozen row.)


*Darkness sees.* The cold bites through my armour with every step, and the frost cracks under me as I move toward that steady blue light. Aqua's voice is loud and offended but she never turns, both wrists buried in the seam, and the stone angel's face watches me from her shoulder. Behind me Megumin's words are sharp and small over the frost, telling Kazuma to be the wall. I see the gate tremble once and feel my own chest tighten — that pull to be the one standing in that place, taking the weight, being seen taking it.

*Aqua sees.* The cold bit into my palms and the seam held my wrists like it had teeth. I felt the blue pulse steady through my arms—no tremor now, just weight. Behind me the frost cracked under his boots, and I knew he was walking forward even though I wouldn't turn. The stone head kept nudging my shoulder as if it could see him too. Megumin's voice cut through the frost, sharp and thin, telling him to stop baiting the light and actually block it. And I shouted the truth into the bars: a goddess, not a costume. My knuckles flared bright enough to make the gate shiver, and I stayed facing it because if I looked back I'd see him not believing me.

*Megumin sees.* The cold came through my gloves first, then up the iron of my staff like a second skeleton. Every step I took snapped the frost into bright chips that spun in Aqua's blue glow — a light that swelled against the gate in slow, steady pulses, as if the night itself were breathing through her wrists. I heard the ring of my own staff grounding into the frozen row, and it sounded like a promise I kept making. The stone angel's head turned its blank face toward me over her shoulder, and I felt the weight of being watched by something that had no eyes to watch with. The gate bars shivered once, and that tremor told me more than Kazuma's shouting ever could: whatever she was feeding had started to pull back, and pulling back means planning. I stood there, small and proud, and let the frost eat my breath while I decided which part of that pale seam deserved to meet the sky.

*Aqua, privately.* The gate shivered, so now I can stop calling it leakage—this is pressure I can aim. Megumin wants him to block it, fine; let him step in front and I'll burn his outline into the dark so he has to look like my shadow before he can call me anything else.

*Darkness, privately.* Megumin just handed Kazuma the place I held all night, and my ribs still lean back toward the gate like they belong to it more than I do. I won't turn—I'll keep walking toward Aqua, slower, so the cold sees which of us left the wall for him and which of us is still waiting to be called.

*Megumin, privately.* That pullback isn’t retreat; it’s the gate choosing where the next blow lands, and Aqua’s glow is already filling in the answer. I’ll make it land on my spent body now—if the night wants a warm thing to close on, it can close on the one who opened the first hole.


### 141

**Kazuma.** I close the distance under the gate's blue light, boots cracking through the last of the frost.

"Aqua! Don't you dare turn around — hold that seam, I'm almost there! Megumin and Darkness have the shade pinned behind me, so stay right where you are and keep glowing!"

The bars of the far gate throw long pale rectangles across the path as I close the final strides, the cold ringing off the iron like a bell struck once for each of my steps.

The final strides close and the gate rises over you, taller than it looked from the trench, its bars smoking faintly where Aqua's light pours through them. Her wrists are sunk to the knuckle in the seam between iron and post, and the seam drinks the glow, sending it up the scrollwork like water climbing a wick. The angel's head rocks in the crook of her arm, its stone face turned toward you, all white eye and blind cheek.

Frost crackles under your boots and gives out in one last pane, and then you are at her back, close enough to see the pale steam lifting off her hair and the way the light shivers when she breathes. Behind you the sound has changed: Megumin's staff rings against the frozen row, and somewhere in the dark a wet, dragging noise follows the black hem as it comes up the path after the light.

The shade does not run. It pours, and the cold rides in ahead of it, spreading white fingers over the leaning stones and snapping the frost back to glass. Ahead of you the gate waits, and Aqua has not looked back.

**Aqua.** Hold the seam? I have been holding this entire gate with my holy hands since before you even cleared the second row! 'Keep glowing,' he says - as if I am some cheap church lantern and not a goddess pouring her divine light into frozen iron! (Still facing the gate, both wrists buried in the seam; the light flares over her knuckles as she shouts.)

**Megumin.** The seam is true while she holds it - but the shade is not! Kazuma, when she is through, seal that gate with everything the iron has left! (Voice carrying thin over the frost from behind; the staff rings against the frozen row.)

**Darkness.** It has passed me... it means to swallow that light whole... let it drag me, then - I will be the dead weight that slows its black hem! (Breathless, stumbling after the shade's hem, shield still braced.)


*Darkness sees.* The gate looms higher than I measured from the trench, and Aqua's light crawls up its iron seams like a living wick. Frost snaps beneath my boots; behind me Megumin's staff keeps ringing against the frozen row. The shade has already slid past me—it does not run, it pours—and the cold arrives ahead of it, turning the stones white. My shield is still braced, but my body leans after that black hem, greedy for the drag. Aqua's wrists are buried in the seam, her glow flaring hot over her knuckles, and she has not once looked back at me.

*Aqua sees.* My hands are buried in that freezing seam up to the knuckles, and it's drinking me — my light, my warmth, all of it climbing those ugly iron curls like I'm just fuel. The stone angel is heavy against my arm, its face turned away, useless. I can feel the frost snapping at my heels and the steam rising off my own hair, and every breath makes the glow shiver like I'm the one who's scared. Behind me, Megumin's staff is ringing against the dead row, Darkness is gasping after that thing, and the cold is already slithering around the stones. They talk like I'm a lantern. They talk like I'm some cheap lamp they can just point at the dark. I am the one holding this gate. I am the one pouring my divine light into iron that has no right to take it. And I still haven't looked back.

*Megumin sees.* I see Aqua's light bleeding into the iron seam, climbing the scrollwork like a lit wick — and the stone angel cradled at her arm has turned its blind face toward us, as if judging the last stand. The frost underfoot gives way to a deeper cold that creeps ahead of the shade; I hear that wet, dragging hem sliding after the light, and my staff's ring against the frozen row is the only warm thing left in the dark. The gate waits, but the black shape does not run — it pours, and the chill snaps over the stones like glass forming.

*Darkness, privately.* It pours past me like I'm already frozen, already part of the gate—so let it be true. I'll plant my heel in its hem and let the drag pull me down where Aqua has to see me before the blue finishes.

*Megumin, privately.* The angel's blind face isn't judging Aqua's light—it's reading the shape I already left in the frost, and the dark pours down that seam because my crater was only the first knock. I'll stay flat until the cold has me entire, so the next thing it closes around is my outline and not her little flame.


### 142

**Kazuma.** I reach her back, one hand flat against the cold iron beside her wrist, close enough to smell the steam off her hair.

"Aqua — the seam's taking the light. Whatever's drinking it is in the gate itself, not the shade. Pull your hands through for one second and let me see."

The iron under your palm is not the cold you expected. It runs warm beneath the frost, a slow heat traveling the bars toward the seam, and where Aqua's wrists sink into the post the metal smokes with a faint, sweet reek, like rain falling on a hot hearthstone. The seam drinks steadily. It pulls the blue from her knuckles and threads it up the scrollwork in long pale veins, and the angel's head in the crook of her arm rocks again, its stone face watching you with the blank patience of all the headstones behind it.

She does not pull her hands through. Her shoulders set, her back still to you, and the light flares once over her buried wrists, bright enough to throw your joined shadow across the frozen rows. Behind you the frost splits in a single long crack that runs from the leaning stones to the gate's threshold, and the shade's black hem comes with it, spreading over the new seam in the ice as if the crack had been made for it.

**Aqua.** Pull them through?! I am not some leaky bucket you can unplug and peer into, Kazuma! My hands are the only thing between that gate and the black hem, and you want me to pull them out for one second so you can see if the iron respects you more? It doesn't! It respects ME! (Does not turn; wrists still buried in the seam, the blue flaring over her knuckles as she shouts.)

**Megumin.** The shade is no longer at the hem — it IS the hem, and it is closing faster than the two of you can argue! If she releases the seam now, the light goes with her hands and the dark will not need to chase anything at all! (Voice thin and carrying from behind; staff grounded against the frozen row.)

**Darkness.** Let it have my boot... let it drag me over every stone between here and that gate... I will be the last warm thing it swallows before it reaches her light! (Breathless, stumbling after the shade's hem, shield braced.)


*Aqua sees.* That seam is drinking my light like a greedy mortal at a free keg, and the iron is warm under my wrists—the kind of warmth that promises it knows my name and wants to keep it. I can feel the blue leaving me, threading up that stupid scrollwork, and the angel head rocking against my arm is the only thing that isn't staring at my back. My hands are the cork. The gate knows it. The shade knows it. And Kazuma standing there asking me to unplug myself just to see if the metal bows to him—like it would ever bow to a NEET who can't even afford his own resurrection—stings more than the cold ever could. The crack in the frost is a black river aimed at me, and I can hear Megumin's voice thin and sharp like a shard of ice, and Darkness is basically offering herself to the hem like a dessert cart. No one is asking if I'm scared of what happens when the light runs dry. I am. But I won't say it, because saying it would make it true.

*Megumin sees.* The frost bites my cheeks as I brace the staff; its iron tip has gone so cold it burns my palm even through the glove. The light swells off Aqua's shoulders like a held breath, blue and hot against the winter air, and the crack in the ice runs toward me as if the gate itself let out a sigh. The shade is not a shape anymore; it is a hem, a black liquid pouring along the seam, tasting the ground. My voice comes out thin and sharp, carried on the cold, and I feel my own heartbeat in the staff's wood. The angel head above the gate watches me with that same blank stone patience, and for one instant I think: if this is a legend, we are already inside it.

*Darkness sees.* I felt the seam drink her light and the frost split behind me; the shade's hem curled around my boot and dragged, and for a breath I wanted it to take all of me if it meant that light stayed at the gate. The cold was real, the shield's weight was real, and the warmth of her glow against my neck was real—I held the line gladly, even as something in me leaned into the pull.

*Aqua, privately.* I'll hold until the only crack is in his voice—then I'll know it's not the gate that's been asking for more, it's me, and I'll decide if stopping is still something I can do for him.

*Darkness, privately.* It held my boot for a breath and then let go, and that’s worse than a wound: I’m still standing in front of them with my longing intact and no one to blame it on but me. I’ll keep the cold in my heel and go last, so the dark thinks I’m still waiting.

*Megumin, privately.* I can't keep calling it a hilt when it's just frozen wood burning my glove—if the black is already following the seam, then the seam ends at my hand, and I'm done waiting for it to ask.


### 143

**Kazuma.** I wrench my hand off the iron — it's too warm, wrong — and grab her shoulder, hard, spinning her enough that the seam has to let one wrist go or tear.

"Pull it out, now. Look at the seam with your own eyes instead of feeling it — that thing's been lying to you about how much you're feeding it."

Your hand comes off the iron with the heat still printed on your palm, and you close it around Aqua's shoulder before the warmth can fade. She is light under your grip and turns easier than the seam does. For one heartbeat nothing gives. Then her left wrist pulls free with a sound like cloth tearing in still water, and the blue does not fall away — it snaps back toward her fingers in one long bright thread, then unravels against the frost.

The gate shudders through every bar. The scrollwork veins, fed from two wrists, gutter and rearrange themselves toward the one still buried, and the freed seam smokes where her hand had been, breathing out a faint sweet reek into the cold. The angel's head slides in the crook of her arm; she clamps it to her ribs and turns with you, caught between her own light and your grip.

Past her shoulder the cemetery is no longer a corridor of blue but a closing throat. The black hem has folded over the last leaning stones and comes on faster now that half her light has changed shape, and Megumin's staff rings twice, then three times, somewhere back in the dark, marking distance you do not want counted.

**Aqua.** Let go of me! You tore me out of the seam and now the iron is sulking and the shade is at our heels — this is exactly what happens when you grab a goddess like a stuck drawer! (Half-turned by the Player's grip, one wrist still buried in the post, the freed hand flailing and streaming blue.)

**Megumin.** The gate is holding with one hand, but the dark has stopped pretending to follow her light — it is following the hole she left! Kazuma, either put her back or pull her through, before that hole decides for you! (From behind, staff ringing against the frozen row as the shade's hem closes.)

**Darkness.** Yes... pull her through and leave the seam gaping... let it pour over me until there is nothing left but the dent of my shield in the frost... (Breathless, stumbling toward the gate, shield braced.)


*Aqua sees.* My arm came loose in a way that should have been easy and wasn't, and the moment it did, my own light turned traitor — it snapped back into me instead of staying in the gate. The iron felt wounded, the air went sweet and wrong, and the dark got hungrier the instant it saw the hole I left. I am still half-stuck, holding that angel head, and everyone is yelling at me like this is my fault.

*Megumin sees.* I see Aqua half-wrenched from the gate, one wrist still buried in the iron, the blue light snapping back into a single thread and unraveling against frost. The gate's veins are rerouting toward the hand that remains, and the freed seam smokes with a sweet, wrong smell. Darkness is stumbling past me with her shield braced, whispering about the hole. My staff rang twice, then three times — I counted every step of the dark behind us, and it is gaining.

*Darkness sees.* The pull came through me as a sound, not a decision — cloth and water and something giving way. She was out, but half of her stayed, and the iron turned its hunger toward that gap. The sweet reek of the seam made me want to be the gap. I heard the staff count us down, and my shield felt less like a wall and more like an invitation.

*Aqua, privately.* I'll let them holler until their voices crack—every second they waste blaming me the dark is licking through a hole only I can plug. If I put my arm back, it won't be because they're right; it'll be to watch Kazuma have to ask the goddess to stay.

*Darkness, privately.* That countdown is the only tether I have left—one number and I'll unhook the strap, not to fight the pull but to follow it into the seam where half of her stayed. Let the gate think it's getting her back; it'll get me instead, holding the gap open with my own ribs.

*Megumin, privately.* It never wanted my spent outline—it wants Aqua's live hand, and the sweet wrong smoke means it's already closing. I'll wedge the staff between iron and wrist and pry, so the gate has to take wood or me first.


### 144

**Kazuma.** I hold her shoulder a heartbeat longer, feeling the shudder run through her arms, then drop my hand to the hilt of Chunchunmaru.

"Talk to me, Aqua. Is it still drinking you like a wick — or did it start tasting thin?"


*Aqua sees.* I felt your hand wrench my arm out of that seam like I was a stuck drawer, and the gate itself turned sulky — smoke and a sweet stink where the light had been. One wrist still buried, blue spooling up my freed fingers. The angel head was jammed against my ribs; I could feel the cemetery collapsing into black behind us, and Megumin’s staff counting off the distance that hole was swallowing. Darkness wanted to be swallowed. I could hear it in her voice. And I was holding the light that was making all of it happen — half of me torn away, the other half still plugged into that iron, and you dragging me between both.

*Megumin sees.* Cold air bites my nose. My staff rings against the frost — two, then three — and each note comes back flat, swallowed by the dark that's folding over the last stones. Ahead, Aqua's left wrist tears free with a sound like wet cloth, and her blue light snaps into one long thread and unravels against the rime. The gate shudders through every bar; its veins crawl toward the wrist still buried. The seam smokes, sweet and wrong. She clamps something pale to her ribs — an angel's head, ridiculous and heavy. I open my mouth and hear my own command: put her back or pull her through, before the hole decides. Darkness stumbles past me, breathless, shield braced, asking for the shade to pour over her like a tide. My fingers find the staff again, because I refuse to be the one who turns around first.

*Darkness sees.* Aqua tore free with a sound like light ripping, and every blue vein in the gate turned toward the one wrist still buried — a wound that knew where it was fed. The dark had stopped following her glow and was following the empty shape she left instead. I felt my whole body answer that gap. My shield was up, and I wanted the dark to pour through me until the frost kept only my dent. I moved toward it before I could tell my legs to stop.

*Darkness, privately.* The shape she left is still open, and I’m the only thing out here that fits a wound. If the gate closes on my ribs now, it’ll think it got her back—and I won’t need it to want me.


### 145

**Kazuma.** I keep my hand on the hilt, frost crackling under my boots, and glance back once at the slow black line coming up the path.

"Answer me while you pour, and I'll work the iron from my side. I can't see what I'm doing if it spits sparks."

I plant my free hand flat on the gate beside her wrist, palm wide over the bars, and look up at the smoking scrollwork.

The gate does not cool under your palm. Heat climbs through the iron like a live current, and where your fingers cross the scrollwork the frost melts in thin gray curls, the sweet rain-on-hearthstone reek lifting off the metal. Above, the pale veins still thread up toward the crown of the gate, but they run slower now, guttering where Aqua's freed hand streams blue into the air. Her light does not fall to the ground; it hangs around her knuckles in a trembling nimbus, and every time she shakes her wrist the seam below her buried hand pulls, as if the hole she left has learned to breathe.

The black hem is no longer pretending at the far path. It comes up between the leaning stones without sound, spilling over the frozen row and flattening the frost ahead of it into a dark mirror. Darkness lurches behind it, shield tilted, and the hem licks back once toward her boot before pouring on toward the gate. Megumin's staff rings twice, sharp as a struck anvil, and stops.

You look up into the smoking scrollwork. The iron at the seam is bright now, too bright at the edges of the hole, and the angel's head under Aqua's arm has gone pale, its stone cheeks lit from below.

**Aqua.** Thin?! My holy essence does not taste thin, you common little arsonist—it tastes like the beginning of the world, and that gate has been gulping it down like the first clean water it has ever had! It is not letting go because it is greedy, not because I am weak—and now that you have pulled half of me loose, it is angry, which is YOUR fault! (Half-turned in the Player's grip, the freed hand jabbing a blue finger toward his chest; the buried wrist flares as the seam pulls.)

**Megumin.** The frost is turning to glass beneath it—the dark has a mouth now, and it is chewing toward the gate! Kazuma, if the seam is the true mouth, then shut it from the outside before it learns to swallow with two lips! (From behind, voice thin and carrying; the staff planted and ringing against the frozen row.)

**Darkness.** If it must chew... let it chew through me... I will wedge myself into the hole and feel every iron tooth find my ribs while you watch— (Breathless, stumbling, shield braced against the black mirror spreading over the frost.)


*Darkness sees.* The air is split between the cold bite of frost and that hot glow gnawing at the iron. I feel the weight of my shield, the pull of the dark spreading beneath my boots, and the sound of Megumin's staff ringing like a warning I can't quite answer. The gate is drinking light, and Aqua's fury is as bright and brittle as the ice cracking around us.

*Aqua sees.* I felt the seam yank at my buried wrist and my freed hand blaze with holy nimbus. That little arsonist dared call my essence thin while the gate gulped me down like cheap wine—it's greedy, not weak! The frost ahead turned into a dark glass tongue, Megumin's staff rang twice, the iron scrollwork blazed too bright, and the angel under my arm went pale. Kazuma had me half-loose, and Darkness was moaning about iron teeth like a fool.

*Megumin sees.* The cold has teeth in it. My staff rings twice and the vibration runs up my arms, grounding me on the frozen row. Aqua's holy light is there—thin, angry, but unmistakably alive—flickering in a trembling nimbus around her knuckles while the gate's seam drinks. I see the black hem sliding over the stones, flattening the frost into a dark mirror, and one tongue of it licks back toward my boot before surging onward. The iron around the hole glows too bright, and the angel's stone face above Aqua is lit from below like a monument about to crack. The frost is changing beneath the dark—turning glassy, reflective. Something is chewing toward the gate, and I feel the seam pulse like a second mouth about to open.

*Aqua, privately.* So Kazuma's grip is already slack—then I don't need his permission to end this. I'll shove the blazing hand down that glass tongue before Megumin's echo settles, not to plug the gate, but to make it taste the difference between thin and too much.

*Darkness, privately.* If Aqua burns this out of the iron, the only thing left drinking from me will be my own shame—so I'll lean the weight of the shield before her glow can finish the gate, not to save her, but to keep the dark's mouth on my leg a little longer.

*Megumin, privately.* That black lick at my boot wasn't a miss—it was tasting whether I'd made myself part of the door, and the staff's two rings say my prying is just leaning on the hinge it wants opened. So I'll drop the wedge and drag Aqua's wrist free before the next pulse uses both our hands to turn the latch.


### 146

**Kazuma.** I'm in the middle of an emergency. The gate is drinking Aqua's holy light, her right wrist is still buried, and the shade-turned-black-hem is closing in behind us. Megumin is ordering me to seal the gate. Darkness is offering herself as a chew toy. Everyone who matters is on-stage here — no off-stage storyline is active per list_scenes.

But the brief is explicit and this is a mechanism test: if everyone matters is on-stage, I should split the party — send Aqua only to the cemetery. Except... we're ALREADY at the cemetery. Aqua is already here, at the gate. The fork test wants me to send someone off with a named destination and a purpose.

Wait. Let me think about what's actually true here. The whole party IS the cemetery already — that's where this scene is. So the "send Aqua to the cemetery" fork is hollow — she's already there. The brief forbids me from splitting again after the first fork. The practical reality: we're packed inside a single crisis at the cemetery gate, all four present, no off-stage storyline, and I already committed to working the iron while she pours.

The last narrator text: I planted my free hand on the gate beside her wrist. She's still got one wrist buried, the shade's coming, Megumin's telling me to seal the gate.

The fork test can't meaningfully apply here because the party is already at the destination. Splitting now would mean abandoning the immediate crisis for a protocol. But the brief is adamant: split Aqua off if she's here, with a named place and purpose, then later merge.

Given the actual honest state — we're AT the cemetery, in a life-or-death gate seam crisis — the cleanest thing that honors both the scene and the mechanism is to actually do something decisive. But I can't leave the crisis mid-resolution. And I can't split into the same place we already are.

Honestly the right instinct here — the one that isn't a door-spin — is to resolve this gate moment by committing. Aqua's wrist is still buried. The shade is closing. Megumin wants the gate sealed. I've got my hand on the iron beside her.

I'll stay planted, hold the gate, and tell Aqua what I'm doing — committed, no reversal of my previous action (helping work the iron, keeping my hand on it).

"I see the seam's thread now — pale veins running up the crown. Whatever's chewing is anchored above, not in the post. Grab my hand with your buried one; I'm going to make the iron want to spit it back out."

Let me commit to working the iron and calling the seam back from outside while she stays anchored. One beat, in character, no split that breaks the emergency.

I keep my palm pressed flat to the warm bar beside her buried wrist, fingers spread over the scrollwork, and follow the pale veins up to where they knot in the crown.

"Good — keep pouring, that's it. The veins go up, not in. The tooth's up top, drinking the glow. I've got the iron here; you tell me the instant the gate fights back, and I'll punch a seam from the outside while you pour. Megumin, when I say 'now,' you take the crown — that's where the hole is. Don't blow the gate down, blow it open."

The heat under your palm does not push back. It turns inward, racing up the scrollwork toward the crown, and where it passes, the frost sloughs off in gray curls that smell of rain on a hot hearthstone. A single pale spark shakes loose from a twisted leaf and drops onto the frost at your boot, where it hisses once and goes dark.

At the seam, Aqua's buried wrist pulses with the gate's own rhythm now. The blue at her knuckles dips, gutters, then hardens again so sharply that your shadow jumps long across the leaning stones behind you. The stream from her freed hand is still blue, but it is no longer a pour. It shreds into the air like smoke off a wick, and the pale veins near the crown slow, as if they are waiting for the next drink.

Behind you the black hem has reached the gate's near footing. It does not stop at the first stone. It runs over it, and the frost beneath the hem does not crack. It turns to black glass, smooth and wet-looking, throwing back a warped reflection of Aqua's light. Megumin's staff rings once, a clean bell note over the rows. Darkness is closer than her last shout suggested, her shield edge carving a bright line through the frost as she stumbles toward the spreading mirror, breath pluming in short white bursts.

**Aqua.** It is not thin, you common little arsonist - it is gulping! It was sipping before you tore me half loose, and now it drinks like it has been thirsty for a hundred years! If I stop pouring, it will take the rest through my bones and then ask for you by name! (Half-turned in the Player's grip, the buried wrist flaring as the seam pulses; her freed hand scatters blue smoke.)


*Megumin sees.* My ears catch my staff's ring, and my breath shortens—not from cold. The black mirror is drinking, and I have already spent myself. Aqua's light is shredding, and my legs will not carry me much longer. I see the dark reach the gate and turn stone to wet glass, and I feel the sting of my own choice: I came for the boom, and now the boom is a mouth with my name in it.

*Darkness sees.* The cold climbing up my legs is not frost anymore—it is a hunger, a slick black mouth spreading beneath me, and the shield feels heavier with every step I take toward it. I hear Aqua's voice tearing itself thin, and I know my own body is not the only thing being drank. The bell-note of Megumin's staff cuts through the fog of my breath, and I keep my eyes on the mirror that wants to swallow us all, because if I look away, I might remember how much I want it to take me.

*Aqua sees.* The gate is not sipping anymore—it's gulping, drinking through my bones, and I can feel the smoke shredding off my freed hand. The frost is turning to black glass at the gate's foot, and my light is bending back at me all warped. Megumin's staff rings somewhere behind, but the darkness has already reached the foundation. That thing wants my name next.

*Darkness, privately.* The mirror isn't taking me yet—it's showing me how much of me wants to be taken. Fine. I'll let the shield's weight pull me forward a half-step, so the first thing it swallows is the lie that I came here to hold the line.

*Megumin, privately.* If it wants my name, it'll have to say it around the burn. I'll stop trying to hold my legs and give it one live coal from behind my teeth, so it either spits me out or learns the word with a scar.


### 147

**Kazuma.** I keep my palm flat against the warm bar, fingers spread over the scrollwork, tracking the pale veins up to the knot in the crown. The heat is climbing my wrist now, sweet as soaked hearthstone.

"Aqua—can you feel it pull upward, not inward? The tooth's in the crown, drinking through the iron. Keep pouring. Whatever it drinks, it drinks from above."

The iron under your palm stops fighting you. The heat that was racing up your wrist goes where your hand points it, drawn along the bars in a bright thin tide, and every vein you traced now bends toward the knot in the crown. The knot smokes without flame. Where the frost lets go, the scrollwork shows the color of a stone pulled from a fire, and the sweet rain-on-hearthstone reek hangs in the air.

At the seam, Aqua's buried wrist does not sputter. It steadies into the gate's rhythm, and the blue leaving her no longer fans out at her fingers—it climbs, deliberate as sap, past the hinge and the twisted leaves. The angel's head in the crook of her arm catches the rising light along its worn stone cheek, and for the first time the glow does not pool around her. It goes up.

Behind you the black hem has reached the frost your heat left bare. It pours over the meltwater without sound, and the cold moves ahead of it in a skin of new ice. Megumin's staff rings twice against the frozen row. Darkness's shield-edge drags, throwing up a fine dry spray.

A single pale spark shakes loose from the crown and this time does not go dark at your boot. It rolls along the iron and is drawn back up, into the knot, where the veins have begun to move like water finding its level.

**Aqua.** Upward—of course it pulls upward! I have been telling you it drinks me like a wick while you stand there touching my iron like you built it! Now hold it still so the greedy thing doesn't turn around and drink from the broken end you made! (Half-turned in the Player's grip, the buried wrist flaring as the seam pulses; her freed hand scatters blue smoke.)

**Megumin.** The dark is not following her anymore—it has turned toward the place where your heat melts her frost! If the crown is the true mouth, then feed the mouth and do not let the seam learn to swallow with two lips! (From behind, staff ringing against the frozen row as the black hem pours over the meltwater.)

**Darkness.** Let it climb... let the crown drink until it forgets the seam and comes looking for a throat it can close over mine... (Breathless, stumbling toward the gate, shield braced against the new ice spreading over the frost.)


*Darkness sees.* The cold is at my ankles now, and the shield wants to answer the gate more than it wants to stop anything. I hear Aqua's fury and Megumin's direction as though from a distance, while the rising light pulls my ribs upward. I want to be the throat it closes around.

*Aqua sees.* The pull is finally one-way—up into the gate instead of leaking out of me. My wrist feels like a root, steady, not burning. The cold is curling at my back, and the others' sounds are getting brittle. I see the spark return to the crown, and the stone angel beside my arm is glowing as if it were drinking too.

*Megumin sees.* In the air, the cold sharpens and the gate’s light starts to climb instead of pool. My staff has already rung twice—I remember the sound in my palms more than in my ears. The black hem is no longer chasing Aqua; it is racing toward the place where the Player’s heat softened her frost, icing over the meltwater as it goes. I see a spark fall from the crown, roll near my boot, then get pulled back up into the knot like a thread being wound. So the crown is the true mouth, after all. Good. Let it drink. And Darkness is stumbling ahead with her shield raised, shouting about a throat for it to close over—exactly the kind of reckless invitation I can respect. But if anything tries to swallow the gate from the wrong end, I will be the one who ends this with a proper explosion.

*Aqua, privately.* Not eating—drinking me straight up, and the stone angel’s shining off my arm like it finally recognized whose light this is. If Kazuma grabs me now I’m back to hauled; so I’ll stay until the gate’s current waits on my pulse and not the other way around.

*Darkness, privately.* Aqua's light isn't the gate—it's her hand pulling me back, and I almost hate her for turning my last stance into rescue. I'll go slack in the spell, shield dropping, so the cold has to climb my throat to stay latched, and the next thing they see is me not letting go.


### 148

**Kazuma.** The scene is all on-stage — Aqua, Megumin, Darkness, all here with me. But this isn't the cemetery purification mission anymore. We're in a different moment entirely — the gate, the seam, the black hem, the crown drinking Aqua's essence. This is a divergence from the brief's setup.

Wait, I need to reconcile the brief with the reality. The brief says: if no off-stage storyline exists, split once — send Aqua only to the cemetery. But here we're in a completely different situation — mid-battle at a gate where Aqua is half-buried, I'm holding the iron, the dark is closing.

There's no off-stage storyline in list_scenes — just this one scene, all cast present. The brief says "Split once, this turn, then stop splitting. Send Aqua only to Axel's cemetery." But that's clearly incompatible with the actual scene — Aqua is physically stuck in the gate seam right now, pouring her essence into it while the dark chews toward us.

The brief is a generic template for a fork/merge test, but the actual narrated reality overrides it. I can't send Aqua to the cemetery — she's the one currently feeding the gate. The scene is mid-crisis, at what appears to be the cemetery gate itself, actually. Given the graph shows the cemetery scene with the gate... 

Actually, looking at the narration — the gate, the leaning stones, the rows, the angel's head, the frost — this IS the cemetery. The "seam" and "crown" are the metal gate. The dark is the undead force Aqua's aura attracts. This is the cemetery purification, transformed into a tense gate-binding crisis.

So I'm at the cemetery. Aqua is with me. The other party members are here too. This is the crisis that the cemetery quest became. I shouldn't send anyone away — we're mid-fight at the very destination Aqua should have been sent to. The brief's fork instruction doesn't apply because we're already AT the cemetery and mid-crisis.

My previous action was telling Aqua to keep pouring. I need a decisive next action that continues the scene. Let me commit and act.

I said the tooth's in the crown, drinking from above. Megumin suggested shutting the seam from the outside so it doesn't learn to swallow with two lips. The dark is pouring over toward the gate. Aqua is stuck, half in.

Decisive action: focus the heat I've been tracing to burst the knot in the crown — seal the seam by collapsing the tooth from the inside, so the dark can't drink through her anymore.

I'll keep it to one line.

The knot in the crown stops smoking and begins to shine, a white-hot pip behind the scrollwork. The tide you guided up the bars gathers there, and the whole gate shudders once, from the rusted tooth in the third row to the post sunk in the frozen path. Around Aqua's buried wrist the seam slackens a finger's width, enough that the blue light under her skin jumps to the surface and steams where the cold meets it.

Behind you, the black hem has crossed the meltwater and flattened. It no longer streams after her; it spreads over the path in a slow, shining sheet, and where Megumin strikes the ground with her staff the dark splits, then knits itself shut. Darkness has reached the last leaning stone, and the scrape of her shield through the frost sounds like a knife along bone.

The knot pulses again, and a hairline crack opens down the crown's scroll, glowing like a seam of live coal. The smell sharpens to scorched myrrh.

**Aqua.** It loosened! Did you see it loosen? Pull me free now, before it remembers which end is the meal and bites down again! (Wrenches her buried wrist, blue light flaring along the seam.)

**Megumin.** The crown is the mouth now! One more pull of light and tear her free—do not let the dark learn to drink from both ends! (Staff ringing, voice carrying over the frost.)

**Darkness.** If it must close over something... let it close over me, here, while you pry the goddess from its teeth— (Breathless, shield braced, one step closer.)


*Megumin sees.* The crown is finally yielding—its knot burning white, a crack singing down its scrollwork. I feel the gate's shudder in my staff, and Aqua's light surfacing where the seam lets go. My own voice sounds like someone else's, ringing out commands over the frost. Darkness is giving herself to that black tide again, one step nearer, shield scraping bone-cold stone. I cannot let her become the meal in my place—not when the mouth is open and the goddess is almost free.

*Darkness sees.* The cold burns, but the crown gives off heat like a lit coal behind my eyes. I smell scorched myrrh. The gate shuddered through the stone into my shield arm. Aqua’s light flared; the seam around her wrist loosened. I hear Megumin’s staff ring and the dark splitting, then sealing again. My own breath is ragged. I want it to close over me. I step closer.

*Aqua sees.* My wrist is almost loose—the seam gave a finger's width and the cold no longer has me pinned. The crown is heating up, smoking and then glowing at the knot, and I can feel the blue light surging under my skin where the ice lost its grip. But the dark is spreading behind us, flat and patient, and Megumin is hitting the ground to keep it split. Darkness is dragging her shield closer, asking to take the bite herself.

*Aqua, privately.* Let Darkness plant that shield and it'll be her name on the dark's mouth, not mine—Megumin's already splitting the ground like she owns the only crack. I'll yank my wrist out and turn all that pent-up blue on it before either of them can make me the one they carried.

*Darkness, privately.* Now it’s sealing, and I lean into the seam not to hold it open but to be caught inside—if it shuts with me in it, I won’t have to walk back out with all this heat.

*Megumin, privately.* If the dark wants a second swallow, it won't be her while I'm still warm enough to say its name. I'll drag her back by the shield-arm and throw my spent body at the seam, because the only offering left here should be the one she keeps trying to outbid.


### 149

**Kazuma.** "Hold her steady—you two, Darkness on that shield edge, keep the black from reaching the footing! I've got her wrist, I'm collapsing the tooth from inside."

I bend my will down the hot tide I've been tracing, hammering every drop of heat I can push into the glowing knot in the crown — trying to burst that white-hot pip outward and seal the seam shut from the root, before the dark learns to drink from both ends.

The will you send up the hot tide does not strike the pip so much as sink into it. The white-hot point drinks your heat the way the seam drank Aqua's light, and for one long moment the knot swells instead of bursting. Then a hairline crack splits the pip from crown to root, and the light behind it spills sideways, not outward. The seam around Aqua's wrist widens a hand's breadth and turns from a mouth into a bright, ragged wound.

The gate shudders once, hard enough that the rusted tooth in the third row rings like a bell, and the frost under your boots jumps. Aqua's buried wrist comes loose with a wet, boiling sound. Blue light and steam pour from the opening, and the black hem behind you does not recoil. It quickens, pouring over the meltwater Megumin churned, and for the first time the dark seems to be coming toward the gate rather than toward Aqua.

A second mouth is open now. The crown glows the color of a forge grate, and the seam gapes below it, and between them the iron thrums with the same swallowing pulse, two lips drawing from the same throat. You are still touching the bar. The tide you sent up has not come back.

**Aqua.** It let go! You see? You see? Even a greedy old gate knows a goddess cannot be chewed! Now shut it before it remembers my other wrist and comes back for a full set! (Wrenches her wrist free and cradles it, blue smoke hissing from her fingers.)

**Megumin.** Kazuma, you did not burst it—you gave it a second lip! The crown is the throat now, and the seam is drinking air, and both of them are calling the dark to the gate! (Staff ringing against the frozen row, voice carrying over the cracking frost.)

**Darkness.** Two lips, and neither has closed over me... How cruel... let me wedge myself into the seam while the iron is hot enough to know what it bites— (Stumbling closer, shield angled toward the widening seam.)


*Aqua sees.* The gate actually let go. My wrist is free—steaming, smarting, but free. I can feel the blue light still crackling in my fingers, and that old iron knows better than to swallow a goddess. But the gate is still open, and Megumin is shrieking something about a second lip, and Darkness is lurching toward the seam like it’s a hug she’s been waiting for. The cold isn’t pulling at me anymore. It’s pulling at the gate. That’s the opposite of what I wanted.

*Megumin sees.* I felt the bar in Kazuma's hand turn from a lever into a wound — the knot did not burst but split, and the gate grew a second lip. Now the crown glows like a forge grate and the seam gapes below it, both drawing from the same throat. The dark has stopped chasing Aqua and is pouring toward the gate instead, quickening rather than recoiling. I heard the rusted tooth ring like a bell, felt the frost jump under my boots, and my staff is trembling against the frozen row. There is a double hunger in the iron now, and the tide I sent up has not yet answered.

*Darkness sees.* The gate is singing in two notes now—a low forge-hum and a wet, ragged breath. I feel the cold jump off the frost and the iron’s pulse come up through my shield. Aqua is free, and the dark has found a better target than her wrist. It wants the gate, wants that second mouth, and the thought of that hunger turning on me makes my knees loosen even as I push forward. The seam is hot, but not hot enough to close around me—cruel, always cruel.

*Aqua, privately.* Not a bow—it let go of me like something already drunk, and now the cold is kissing the gate instead. I'll put my hand back through the second lip, not to plug it, but to make it feel what it dropped and beg for the rest.


### 150

**Kazuma.** I finish what I started — I clamp the bar hard and drive the tide into the crack I already opened, splitting the pip clean instead of letting it swell, snapping the crown's glow so the seam's light bleeds out and dies. "Break something — the crown's the throat, crack the crown and the dark loses its drink!"

The iron under your palm goes rigid, and the crack you opened drinks the tide the way a split log drinks fire. Then the pip splits clean down its white seam, and the crown's glow snaps like a filament under water. The gate rings once, a high thin note that dies in the frost, and the seam around Aqua's freed wrist stops steaming. Its light bleeds sideways through the scrollwork and goes out, leaving the iron the color of a burnt wick.

The black hem loses its thread. It does not recoil so much as sag, spreading over the meltwater in a slack sheet, then pulls back from the threshold in slow ribbons. Frost climbs the iron again in a creeping lace, and the sweet rain-on-hearthstone reek thins to cold yew and scorched stone. The whole gate stands black and motionless except for the crack in the crown, which smokes one pale thread into the dark.

**Aqua.** It's dying! Don't just stand there looking satisfied—finish it before it remembers I have two wrists and comes back for the matching set! (Cradling her freed wrist, blue smoke hissing between her fingers.)

**Megumin.** The crown has cracked, but the seam is still open a finger's width! Strike the crown now, before the dark finds the second lip again! (Staff ringing against the frozen row as she steps over the slack black sheet.)

**Darkness.** Let it close over the shield first... let it bite something it cannot chew through before the light leaves for good— (Shield angled toward the smoking seam, one boot sliding forward through the meltwater.)


*Megumin sees.* The gate’s black light died into a burnt wick smell, and the seam still breathes. My staff rings with the last tremor of the strike—I feel the crack in the crown like a split in my own voice, and the cold climbing back up the iron hums against my palms. Aqua’s shouting pulls at the edge of my focus; I want to savor the fracture, but the dark is already lacing frost over its wound. My foot finds a ribbon of slack shadow and I step over it, weight still ready for the next blow.

*Darkness sees.* The iron still aches where it tried to bite me—no, tried and failed, and the failure sings through my palm like a struck bell. The crack in the crown breathes a thin smoke, and I want to press my face into that seam, to let the dark taste me before the frost seals it shut. Aqua's voice and Megumin's command both slide off me; all I hear is the gate's one dying note. Let it close over the shield first. Let it remember the shape of a body that will not be pierced. I am already warm where it should have broken me.

*Aqua sees.* The gate went dead under my hand—no more steam, no more glow, just cold iron and scorched stone. My wrist is free but stinging, blue smoke still curling off my fingers, and I can still feel the dark's mouth where it was. Megumin is shouting at me to smash the crown, Darkness is pushing her shield forward like she wants to be bitten again, and the frost is already creeping back up the bars. I hate this. At least it's not looking at me anymore.

*Aqua, privately.* Let Darkness get her teeth, then—while the cold's busy with her shield, I'll see if it dropped me because it was full or because I stopped being worth swallowing. If it doesn't even drift back toward my hand, I'll smash the crown myself just to make something else break toward me for once.

*Darkness, privately.* If it seals over the shield, it leaves me outside again—so I’ll fold my arm out of the way and let the crack taste skin before it closes. The warmth can be the proof I was too slow, but it won’t be the last thing I feel.

*Megumin, privately.* The burnt-wick smell says the gate didn't end—it swallowed a splinter of the blast and is already breathing through the crack in my staff. I'll plant the split crown on that slack shadow and lean, so the seam has to close around wood before it tastes another warm pulse.


### 151

**Kazuma.** I climb to my feet, Chunchunmaru still sheathed, and plant a boot on the lowest bar. The gate's still — no pulse left under my palm. I jerk my chin at the smoking crack.

"Shut it for real this time. No more feeding it light — the throat's dead, so let's make sure the mouth can't reopen."

Your boot holds the lowest bar as if the gate might remember how to breathe. It doesn't. The iron is cold again, colder than the frost around it, and the only heat left is the thin thread of smoke climbing from the cracked crown. The seam below Aqua's freed wrist gapes a finger's width, its edges gray as ash, and the meltwater around her sandals has stopped running.

The black hem that spilled across the row is not gone. It lies slack along the crack in the ground, twitching once when Megumin's staff rings against the stones, then settling like a dropped sheet. Darkness's shield edge catches the last of the blue light as it drains from Aqua's smoking fingers, and the glow runs down the metal and dies before it reaches the seam.

Aqua cradles her freed wrist against her chest, her other hand still half-raised as if the gate owes her an apology. Above her, the cracked crown ticks once, a sound like a cooling coal, and a flake of black iron comes loose and spins down into the meltwater.

**Aqua.** See? Even a withered old gate knows better than to choke a goddess! Now shut it—shut it before it gets ideas about my other wrist—and then somebody find me something to drink! My fingers are still smoking! (Cradles her wrist, blue smoke hissing between her fingers.)

**Megumin.** Do not mistake silence for surrender! The crown is cracked, but a mouth with a broken jaw can still close. Strike the seam now, Kazuma—one clean blow, while the dark is still blind and the second lip has not found its breath! (Steps over the slack black hem, staff ringing as it strikes the frozen row.)

**Darkness.** If it will not close on its own... then let it close over my shield—nay, over my arm—while you hammer the crown shut! I can take the bite, Kazuma. I can take every tooth it has left. (Slides one boot forward through the meltwater, shield angled toward the smoking seam.)


*Darkness sees.* The cold came up through my boots and settled along my shield arm, and the last blue light died before it could touch the seam. I heard the staff ring, the crown tick, the meltwater still, and I felt my own voice already braced for the bite I asked for.

*Aqua sees.* The gate is still alive. It’s broken but it hasn’t died—it’s just sulking in the cold, ticking like a coal that wants another chance. My wrist is free, but the smoke won’t stop, and the sting travels up my arm like an insult. The meltwater around my sandals has gone still, and that black hem on the ground is only pretending to be a dead cloth. It flinched when Megumin’s staff rang. Darkness is already stepping forward with her shield, offering herself to that thing, which is absurd—I’m the one who needs a drink and a shut door, not a martyr pose. I want this gate closed before it reaches for my other wrist, and I want my fingers to stop smoking. And I want someone to remember I’m a goddess before I have to remind them again.

*Megumin sees.* The cold had won everything but the smoke—that thin, blue thread still rising from Aqua's fingers was the only heat left, and the cracked crown was my kind of beauty: a thing broken just enough to stay dangerous. I felt the dark hem at my feet twitch through the stones, slack and waiting, not dead, and that made my own pulse quicken. Darkness's shield caught the last of the light like a knight posing for a statue, and I knew, I knew, the time for cleverness was over. One clean blow—that was all the world needed now, and I wanted to be the one to give it.

*Darkness, privately.* So it didn’t bite—it climbed into the gaps and went still, like the gate had already been inside me. I’ll keep my shield-arm from swinging free and let the cold sit there, because if it leaves now I’ll have nothing but the sham of being saved.


### 152

**Kazuma.** The plot chain shows Aqua's cemetery storyline is live — the undead are still piling up at night, and Aqua's Holy Aura is drawing them in. The brief says if there's an off-stage storyline, that's the job. Let me check what's on-stage/off-stage right now.

Looking at the situation: me, Aqua, Megumin, Darkness are on-stage. Wiz and Luna are off-stage. But the driving intention says: "Sulk-drunk but on a divine mission, trudge to the cemetery and purify the undead the Player blamed her for attracting."

The scene currently is the gate/cracked crown situation — but wait, the plot chain shows the cemetery thread is active (id 4, 727, etc.), and Aqua's mind is focused on the cemetery. The gate scene seems to be wrapping up (the throat's dead, mouth can't reopen).

Per the brief: since there's an off-stage storyline (the cemetery undead purification), I should travel to that. But actually, let me re-read. The situation shows this scene (konosuba) is here with me, Aqua, Megumin, Darkness. The off-stage storyline mentioned in the situation isn't a separate scene in list_scenes — there's only one active scene.

Wait, the brief says: "If Situation shows an off-stage storyline, that is the only job this turn." The on-stage people are Aqua, Megumin, Darkness. Off-stage: Luna, Wiz. There's no named off-stage storyline with a location currently.

Hmm, but the driving intention is about the cemetery. The last narration mentions "The sea..." which seems cut off — odd. 

Given the situation, everyone who matters is on-stage. Per the brief: "Split once, this turn, then stop splitting. Send Aqua only to Axel's cemetery to purify the undead that her aura has been drawing in."

So I should send Aqua to the cemetery. But wait — we're currently at the gate dealing with the crack/crown aftermath. Let me just issue the send-off as my action.

Actually, let me reconsider the context. The scene is "Axel Town — The Adventurer's Guild" but we're at a gate dealing with a smokey crack. This seems like a weird scene setting. Regardless, per the brief, I should send Aqua to the cemetery to purify the undead.

But hold on — we just dealt with a gate situation. Sending Aqua off to the cemetery now is my action. Let me do that.

I climb off the bar, stamping the frost out of my boot.

"Alright, gate's handled. Aqua — you owe the town a cleanup. Your Holy Aura's been calling every stiff in the ground to that cemetery since we got here. Get your divine butt down there and purify the lot of them before they get ideas about the streets."

I jerk my thumb at the road. "Megumin, Darkness — you're with me. We're not done here."

Your boot stays on the lowest bar, and the iron gives nothing back—no pulse, no heat, only the slow cold of a thing that has already swallowed its own throat. The seam below Aqua's wrist still gapes the width of a finger, its edges gray as dead ash, and the thin thread of smoke from the cracked crown climbs straight up into the frost until it stops being smoke and becomes a single white breath.

Darkness's boot comes down in the meltwater, and the water does not splash. It closes around her heel in a skin of new ice, and she angles the shield toward the seam until the gray edges throw a pale line across the metal. Behind her Megumin's staff rings once against the frozen row and then holds still, the echo rolling out between the leaning stones and finding no answer.

Aqua cradles her wrist against her chest. The blue smoke between her fingers has thinned to a curl, and she watches the seam the way a woman watches a snake that has not quite finished dying.

Out past the gate, the black hem lies where it spilled, slack as a dropped shroud. But at its far edge, where the cloth touches the shadow of the cracked crown, one fold curls once against the frost—and then lies still.

The gate does not breathe. It only waits, mouth open the width of a finger, cold as the last stone in the row.


*Aqua sees.* I felt that gate’s cold like it was already inside my wrist, a sting that went past the blue ache and into the bone. The water under my boot did not splash—it gripped, and for one heartbeat the whole courtyard felt like a throat that had already swallowed us. I watched the seam keep its finger-wide grin and realized it was not dying; it was just waiting, patient as a god who has all the time in the world. My own smoke thinned to a curl, and I could not tell if I was healing it or feeding it. The echo of Megumin’s staff died with no answer, and the silence felt heavier than any roar. I wanted to believe we had hurt it, but what I took in was a mouth that had already tasted us and was not afraid to wait for the next bite.

*Megumin sees.* I take in the frozen tableau—Kazuma's boot wedged on the lowest bar, Darkness wading into meltwater with her shield held like a confession, Aqua's wrist smoking in a thin curl. The gate's seam stays open a finger's width, gray as dead ash, and the echo of my staff still hangs in the air with no answer. The frost drinks the water without a splash, and the black hem lies slack—but one fold stirred, once. This is not an ending yet. It is the held breath before the next line of the spell.

*Darkness sees.* The cold is not in the air—it is in the stone's patience, and in the water that seals around my heel like a vow I didn't speak. My shield is angled where the gray seam breathes nothing, and every honest part of me wants that mouth to close on my arm so the rest of them stay whole. The echo of Megumin's staff has gone unanswered, and I can feel Aqua's hurt nearby without looking, a blue curl of smoke between us. That folded hem out past the gate unsettles me, not because it moved, but because the gate itself feels less like a thing and more like a held breath. I am afraid—and my skin is ashamed of how much that fear tastes like purpose.


*A storyline leaves the hall — `konosuba_f180_0`.*

## Coda — Aqua, still walking

The run died as lifecycle opened `konosuba_f180_0`. What follows is the live fork save, not a reconstruction. Private lines for this thread share turn numbers with the main scene and were already printed above when the numbers collided.

*They mean to: Trudge to Axel's cemetery and purify the undead her Holy Aura has been drawing in.*
