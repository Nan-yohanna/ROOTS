/*
Vegetable/plant database for the Roots assistant.
Moisture values are 0-100% to match the ESP32 sensor scale and the
Settings page's minMoisture/maxMoisture range sliders.
*/

const plants = [
{
name: "Tomato",
aliases: ["tomato", "tomatoes"],
minMoisture: 60,
maxMoisture: 80,
frequency: "Water daily in dry season, every 2 days once established; avoid wetting the leaves.",
stage: "70-85 days to maturity",
tips: "Mulch around the base to reduce evaporation and blossom-end rot.",
issues: "Blossom-end rot usually means inconsistent watering, not a calcium problem — keep moisture steady.",
},
{
name: "Pepper",
aliases: ["pepper", "peppers", "bell pepper", "tatashe", "rodo", "scotch bonnet", "ata rodo"],
minMoisture: 55,
maxMoisture: 75,
frequency: "Water every 1-2 days; peppers dislike waterlogging more than dryness.",
stage: "90-150 days depending on variety",
tips: "Let the top 2cm of soil dry slightly between waterings to encourage fruit set.",
issues: "Flower drop is often from overwatering or heat stress, not underwatering.",
},
{
name: "Onion",
aliases: ["onion", "onions", "alubosa"],
minMoisture: 50,
maxMoisture: 65,
frequency: "Water every 2-3 days; reduce sharply 2-3 weeks before harvest to help bulbs cure.",
stage: "90-120 days",
tips: "Onions are shallow-rooted, so frequent light watering beats infrequent heavy watering.",
issues: "Soft, rotting bulbs usually mean the soil stayed too wet near harvest.",
},
{
name: "Okra",
aliases: ["okra", "okro"],
minMoisture: 50,
maxMoisture: 70,
frequency: "Water every 2 days; fairly drought-tolerant once established.",
stage: "55-65 days",
tips: "Okra tolerates Nigeria's dry spells well but yields more pods with consistent moisture.",
issues: "Tough, fibrous pods usually mean the plant was water-stressed during pod development.",
},
{
name: "Ugu (fluted pumpkin)",
aliases: ["ugu", "fluted pumpkin", "fluted pumpkin leaf", "pumpkin leaf"],
minMoisture: 65,
maxMoisture: 85,
frequency: "Water daily; ugu has broad leaves and loses moisture fast.",
stage: "70-90 days (leaves harvestable from 5-6 weeks)",
tips: "Give partial shade in peak dry season to reduce leaf scorch.",
issues: "Wilting by afternoon even with wet soil can mean root rot from prior overwatering.",
},
{
name: "Waterleaf",
aliases: ["waterleaf", "water leaf", "gbure"],
minMoisture: 70,
maxMoisture: 90,
frequency: "Water daily, sometimes twice in very dry heat — a high-water-demand leafy crop.",
stage: "40-50 days",
tips: "Grows best in constantly moist, slightly shaded beds.",
issues: "Yellowing leaves usually signal underwatering, not nutrient deficiency, for this crop.",
},
{
name: "Spinach / Amaranth",
aliases: ["spinach", "amaranth", "efo tete", "tete", "green"],
minMoisture: 60,
maxMoisture: 80,
frequency: "Water every 1-2 days; shallow roots dry out fast.",
stage: "25-40 days",
tips: "Harvest outer leaves regularly to encourage continuous growth.",
issues: "Bolting (flowering early) is often triggered by heat stress combined with dry soil.",
},
{
name: "Cabbage",
aliases: ["cabbage"],
minMoisture: 60,
maxMoisture: 75,
frequency: "Water every 2 days; needs consistent moisture especially while heads are forming.",
stage: "70-90 days",
tips: "Irregular watering causes heads to split.",
issues: "Split heads mean a dry spell was followed by heavy watering — keep it consistent.",
},
{
name: "Carrot",
aliases: ["carrot", "carrots"],
minMoisture: 55,
maxMoisture: 70,
frequency: "Water every 2-3 days; deep but infrequent watering encourages long roots.",
stage: "70-100 days",
tips: "Loose, sandy soil plus even moisture gives straighter roots.",
issues: "Forked or stunted roots often mean compacted soil or inconsistent watering, not overwatering.",
},
{
name: "Cucumber",
aliases: ["cucumber", "cucumbers"],
minMoisture: 65,
maxMoisture: 85,
frequency: "Water daily; cucumbers are over 90% water and sensitive to dry spells.",
stage: "50-70 days",
tips: "Mulch heavily; irregular watering causes bitter-tasting fruit.",
issues: "Bitter fruit is a classic sign of drought stress during fruiting.",
},
{
name: "Lettuce",
aliases: ["lettuce"],
minMoisture: 65,
maxMoisture: 80,
frequency: "Water daily in light amounts; shallow roots dry quickly.",
stage: "30-50 days",
tips: "Best grown in cooler, shaded parts of the dry season.",
issues: "Bitter, bolted lettuce usually means heat plus underwatering together.",
},
{
name: "Garden egg (Eggplant)",
aliases: ["garden egg", "eggplant", "aubergine", "igba"],
minMoisture: 55,
maxMoisture: 75,
frequency: "Water every 1-2 days, more frequently once fruiting starts.",
stage: "80-100 days",
tips: "Consistent moisture during flowering improves fruit set.",
issues: "Dropped flowers often mean moisture stress or extreme heat, not pests.",
},
{
name: "Bitter leaf",
aliases: ["bitter leaf", "bitterleaf", "onugbu"],
minMoisture: 55,
maxMoisture: 75,
frequency: "Water every 2-3 days; fairly hardy and drought-tolerant once established.",
stage: "Leaves harvestable from 10-12 weeks",
tips: "Established plants tolerate irregular watering better than most leafy greens.",
issues: "Sparse leaf growth is more often a nutrient issue than a water issue for this crop.",
},
{
name: "Jute (Ewedu)",
aliases: ["jute", "ewedu", "jute mallow"],
minMoisture: 65,
maxMoisture: 85,
frequency: "Water daily; needs consistently moist soil for tender leaves.",
stage: "40-60 days",
tips: "Frequent light watering keeps leaves tender rather than fibrous.",
issues: "Tough, stringy leaves usually mean the crop went through dry spells.",
},
{
name: "Celosia (Soko)",
aliases: ["celosia", "soko", "soko yokoto"],
minMoisture: 60,
maxMoisture: 80,
frequency: "Water every 1-2 days.",
stage: "35-45 days",
tips: "Similar water needs to amaranth; harvest young for tender leaves.",
issues: "Early flowering often means heat stress combined with inconsistent watering.",
},
];

export function findPlant(query) {
const q = query.toLowerCase().trim();
return plants.find((p) => p.aliases.some((alias) => q.includes(alias)));
}

export function listPlantNames() {
return plants.map((p) => p.name);
}

// ---------------------------------------------------------------------
// Intent detection (ported from the standalone Roots Assistant chatbot)
//
// Lets the UI tell narrow, single-field questions ("how often should I
// water tomato?", "okra issues?", "list crops") apart from general /
// full-info requests ("tomato", "tell me about okra"). This is what the
// assistant page uses to decide whether to show the full plant card
// with the "Apply settings" button, or just a short text reply.
// ---------------------------------------------------------------------
export const INTENTS = {
moisture: ["moisture", "soil moisture", "wet", "dry", "%", "percent"],
frequency: ["how often", "frequency", "when should i water", "watering schedule", "how many times"],
tips: ["tip", "advice", "help", "grow", "growing"],
issues: ["problem", "issue", "wilt", "yellow", "rot", "dying", "dead", "disease", "pest"],
list: ["what vegetables", "list", "which crops", "what can i grow", "supported"],
greeting: ["hello", "hi", "hey", "good morning", "good afternoon"],
};

// Intents whose replies are narrow (single field) and should NOT render
// the full plant card / Apply button.
export const NARROW_INTENTS = ["moisture", "frequency", "tips", "issues"];

export function detectIntent(query) {
const q = query.toLowerCase().trim();
for (const [intent, keywords] of Object.entries(INTENTS)) {
if (keywords.some((k) => q.includes(k))) return intent;
}
return null;
}

export default plants;
