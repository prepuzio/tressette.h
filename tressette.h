/* 		Copyright 2026 (c) Francesco Orlando
 * */
#ifndef TRESSETTE_H
#define TRESSETTE_H

typedef signed char     int8_t;
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int	uint32_t;

#ifdef __linux__
#include <sys/random.h>

static inline uint32_t crypto_rand32(void){
	uint32_t v;
	getrandom(&v, sizeof v, 0);
	return v;
}

#define RAND() 	(crypto_rand32())
#define TRESSETTE_MEMCPY(dest, src, size) __builtin_memcpy((dest), (src), (size))
#define TRESSETTE_MEMSET(dest, value, size) __builtin_memset((dest), (value), (size))
#else
/* TODO here roll your own rand, memset and memcpy */
#endif

#define MAX_PLAYERS		8
#define MAX_TABLE		MAX_PLAYERS
#define MAX_HAND		13
#define MAX_DECK		DECK_SIZE

#define DECK_SIZE	40
#define END_OF_DECK	(CARD(0xf, 0xf))

typedef uint8_t card_t;
enum suit_t {
	BASTONI, DENARI, SPADE,	COPPE,	
};
enum rank_t {
	QUATTRO, CINQUE, SEI, SETTE, 
	FANTE, CAVALLO, RE,
	ASSO, DUE, TRE,
};

#define SUIT_MASK			0xF0
#define RANK_MASK			0x0F
#define SUIT(c)				((c) >> 4)
#define RANK(c)				((c) & RANK_MASK)
#define CARD(suit, rank)		(((suit) << 4) | (rank))

/* maps directly to 
 * - 0 for SCARTINE (between 4 and 7)
 * - 1 for FIGURA + DUE/TRE 
 * - 3 for Aces 
 * points are expressed as multiples of 3 */
#define CARD_POINTS(c) \
	((RANK(c) >= FANTE && RANK(c) <= TRE) + (2u*(RANK(c) == ASSO)))

/* we need point_t to be compatibile with card_t */
typedef card_t point_t; 
#define POINT_VALUE(p)		(p/3)
#define RETE			3
#define PISELLONE		33

#define SUIT_CMP(c1, c2)   	((((c1) ^ (c2)) & 0xF0) == 0)
#define CARDCMP(c1, c2)\
	(SUIT_CMP(c1, c2) && (RANK(c1) >= RANK(c2)))
#define DECK_WINNING_CARD_IDX(d) \
	(_deck_winning_card_index((d), (d##_len)))

// busso payload 
#define BUSSO_RANK_MASK			0x03 // ace, two or three 
#define BUSSO_SUIT_MASK			0x0C // napoli suit or mancante
#define BUSSO_KIND_MASK			0x30  // 0011 0000
#define BUSSO_KING(b)			((b) & BUSSO_KIND_MASK)
#define BUSSO_SUIT(b)			((b) & BUSSO_SUIT_MASK)
#define BUSSO_RANK(b)			(((b) & BUSSO_RANK_MASK) + ASSO) 
#define BUSSO_ENCODE_RANK(r)		((r) - ASSO)

/* example usage: busso(BONGIOCO, BASTONI, ASSO) */
/* example usage: busso(NAPOLI, BASTONI, NULL) */
/* example usage: busso(STELLA, NULL, ASSO) */
#define BUSSO(kind, suit, rank)		((kind) | (((suit) << 2) | (BUSSO_ENCODE_RANK(rank))))
enum {
	NAPOLI =	0x10,
	BONGIOCO =	0x20,
	STELLA =	0x30,
};

// return message for dispatching
typedef uint8_t message_t;
#define MAKE_MESSAGE(type, data)  ((type << 7) | (data & 0x7F))
enum {
	ERROR,
	OK,
};
enum errors {
	ERR_UNKNOWN,
	ERR_NOCARD,
	ERR_NOTPLAYING, 
	ERR_AUTH,
	ERR_COMMAND,
};
enum oks {
	OK_FIRST,	// can play card because it's first turn
	OK_PIOMBO,	// can play card beacuse player is piombo
	OK_SUIT,	// can play card because same suit 
};

typedef uint8_t game_state_t;
enum {
	GAME_ERROR,
	GAME_INIT,
	GAME_PLAYER_TURN,
	GAME_END,
};

typedef uint8_t game_settings_t;
enum {
	GAME_BUSSO_ENABLED   = 1 << 0,
	GAME_SQUADRE_ENABLED = 1 << 1,
	GAME_PERDERE_ENABLED = 1 << 2,
	GAME_MONTE_ENABLED = 1 << 3,
};

/* action */
typedef uint8_t action_t;
enum {
	PLAY 	= 0u << 6,
	BUSSO 	= 1u << 6,
	MONTE	= 2u << 6,
};
#define GET_ACTION(a)   ((a) & 0xC0) // first two bits
#define GET_PAYLOAD(a)  ((a) & 0x3F) // last 6 bits

#define STEP(step, p) \
	(step | GET_PAYLOAD(p))

/* TODO implements monte */
// when a MONTE message get dispatched succesfully should set GAME_END
// and quit the game 
typedef card_t monte_t;
enum {
	TRESSETTE, // mandare a monte per 3 sette in mano
	PUNTOFIGURA, // esattamente un punto e una figura in mano
};

/* TODO implements busso */
// when a BUSSO get dispatched successfully, the storage byte gets updated 
// with the kind of busso played (so it can't be played twice) 
typedef uint8_t busso_storage_t;
enum { 
	SLOT_NAPOLI_BASTONI		= 1u << BASTONI,
	SLOT_NAPOLI_SPADE		= 1u << SPADE,
	SLOT_NAPOLI_DENARI		= 1u << DENARI,
	SLOT_NAPOLI_COPPE		= 1u << COPPE,
	SLOT_BUSSI_DISABLED		= 1u << 4,
	SLOT_BONGIOCO_ASSO		= 1u << 5, 
	SLOT_BONGIOCO_DUE		= 1u << 6,
	SLOT_BONGIOCO_TRE		= 1u << 7,
};
#define SLOT_BONGIOCO(r)			(1u << 4 + BUSSO_RANK(r))
#define SLOT_NAPOLI(s)				(1u << (s))

#define BUSSO_SET(store, slot)			((store) |= (slot))
#define BUSSO_GET_IF_ENABLED(store, slot) \
	(((store) & SLOT_BUSSI_DISABLED) ? (0) : ((store) & (slot)))

struct player_t {
	card_t hand[MAX_HAND];
	int8_t hand_len;

	card_t deck[MAX_DECK];
	int8_t deck_len;

	busso_storage_t busso_storage; 
	point_t bussi; 
};

#define PLAYER_DROP_CARD(p, c)		_player_delete_card((p), (c))

struct game_t {
	game_state_t state;

	card_t deck[DECK_SIZE];
	int8_t deck_len;

	card_t table[MAX_HAND];
	int8_t table_len;

	struct player_t players[MAX_PLAYERS];
	int8_t players_len;
	int8_t current_player;

	point_t (*point_calculator)(card_t);

	game_settings_t settings;
};

#define NEW_GAME_STATIC(name, settings)				\
	struct game_t __g##name = {0}, *(name) = &__g##name;

#define GAME_TABLE_ADD_CARD(g, c) \
	((g)->table[(g)->table_len++] = (c))
#define GAME_CURRENT_PLAYER(g) \
	(&(g)->players[(g)->current_player])
#define GAME_NEXT_PLAYER(g) \
	(((g)->current_player + 1) % ((g)->players_len))
#define GAME_ADVANCE_PLAYER(g) \
	((g)->current_player = GAME_NEXT_PLAYER(g))
#define GAME_TURN_END(g) \
	((g)->table_len == (g)->players_len)
#define GAME_WINNER(g) \
	(((g)->current_player + DECK_WINNING_CARD_IDX((g)->table)) % (g)->players_len)

#define DECK_DRAW(d) _deck_draw((d), &(d##_len))
#define DECK_VALUE(g, d) \
	((reduce_deck_points(d, d##_len, (g)->point_calculator))/3)

typedef point_t (*point_calc_t)(card_t);

static inline point_t card_points_classic(card_t c){
	return CARD_POINTS(c);
}

static inline point_t card_points_perdere(card_t c){
	// optimized ace of bastone values 33 points (33/3 = 11)
	return (c == CARD(BASTONI, ASSO))*30 + CARD_POINTS(c);
}

static inline void deck_init(card_t *d) {
	card_t *c = d;
	for (card_t s=0; s<=COPPE; s++)
		for (card_t r=0; r<=TRE; r++)
			*c++ = CARD(s, r);
}

static inline void deck_random_swap(card_t *d){
	const uint16_t r = RAND() % (DECK_SIZE * DECK_SIZE);
	const card_t tmp = d[r / DECK_SIZE];
	d[r / DECK_SIZE] = d[r % DECK_SIZE];
	d[r % DECK_SIZE] = tmp;
}

static inline void deck_scramble(card_t *d, int steps){
	for(int i=0; i<steps; i++)
		deck_random_swap(d);
}

static inline uint8_t _deck_winning_card_index(card_t *d, uint8_t size){
	uint8_t idx = 0;
	for (int i=1; i < size; ++i)
		if (CARDCMP(d[i], d[idx]))
			idx = i;
		
	return idx;
}

/* remove first occurence of c card inside p's hand (type deck = array of cards) */
static inline void _player_delete_card(struct player_t *p, card_t c){
	for (size_t i = 0; i < p->hand_len; i++) 
		if (c == p->hand[i]){
			p->hand[i] = p->hand[p->hand_len - 1];
			p->hand_len--;
			return; 
		}
}

// check if card inside player hand
static inline uint8_t _player_has_card(struct player_t *p, card_t c){
	for (size_t i = 0; i < p->hand_len; i++) 
		if (c == p->hand[i])
			return 1; 

	return 0;
}

static inline uint8_t _player_piombo(struct player_t *p, enum suit_t s){
	for (size_t i = 0; i < p->hand_len; i++) 
		if (s == SUIT(p->hand[i]))
			return 1; 
	return 0;
}

static inline void _game_score(struct game_t *g){
	g->current_player = GAME_WINNER(g);
	struct player_t *p = GAME_CURRENT_PLAYER(g);

	TRESSETTE_MEMCPY(p->deck + p->deck_len,
			g->table,
			g->table_len * sizeof(card_t));
	p->deck_len += g->table_len;
	g->table_len = 0;
}

static inline void _game_play_card(struct game_t *g, card_t c){
	PLAYER_DROP_CARD(GAME_CURRENT_PLAYER(g), c);
	GAME_TABLE_ADD_CARD(g, c);

	if (GAME_TURN_END(g))
		_game_score(g); // if last turn -> score points
	else 
		g->current_player = GAME_NEXT_PLAYER(g);  // else next player
}


static inline message_t _game_apply_busso(struct game_t *g, card_t c){
	// TODO
}
static inline message_t _game_apply_monte(struct game_t *g, card_t c){
	// TODO
}

static inline message_t _game_apply_play(struct game_t *g, card_t c){
	card_t s;
	struct player_t *p = GAME_CURRENT_PLAYER(g);

	if(!_player_has_card(p, c)) 
		return MAKE_MESSAGE(ERROR, ERR_NOCARD);

	if(g->table_len == 0){
		_game_play_card(g, c);
		return MAKE_MESSAGE(OK, OK_FIRST);
	}

	if(_player_piombo(p, SUIT(c))){
		_game_play_card(g, c);
		return MAKE_MESSAGE(OK, OK_PIOMBO);

	}

	// can play card if suit is the same
	s = g->table[0]; 
	if (SUIT(s) == SUIT(c)){
		_game_play_card(g, c);
		return MAKE_MESSAGE(OK, OK_SUIT);
	}

	return MAKE_MESSAGE(ERROR, ERR_UNKNOWN);
}


static inline message_t game_dispatch(struct game_t *g, action_t a){
	// error not playing
	if (g->state != GAME_PLAYER_TURN) return MAKE_MESSAGE(ERROR, ERR_NOTPLAYING);

	switch (GET_ACTION(a)){
		case PLAY:
			return _game_apply_play(g, GET_PAYLOAD(a));
		case BUSSO:
			return _game_apply_busso(g, GET_PAYLOAD(a));
		case MONTE:
			return _game_apply_monte(g, GET_PAYLOAD(a));
		default: 
			return MAKE_MESSAGE(ERROR, ERR_COMMAND);
	}
}

static inline message_t game_dispatch_auth(struct game_t *g, uint8_t idx, action_t a){
	if (idx != g->current_player)
		return MAKE_MESSAGE(ERROR, ERR_AUTH);
		
	return game_dispatch(g, a);
}

static inline int8_t game_hand_len(struct game_t *g){
	switch (g->players_len) {
		case 2: return 10;
		case 3: return 13;
		case 4: return 10;
		case 5: return 8;
		case 8: return 5;
		default: return -1;
	}
}

static inline int8_t game_add_player(struct game_t *g){
	if (g->players_len >= MAX_PLAYERS)
		return -1;

	struct player_t *p = &g->players[g->players_len];
	TRESSETTE_MEMSET(p, 0, sizeof *p);

	return g->players_len++;
}


static inline point_t reduce_deck_points(card_t *deck, int8_t deck_len, point_calc_t fn){
	point_t acc = 0;
	for (int i = 0; i < deck_len; i++)
		acc += fn(deck[i]);

	return acc;
}

static inline point_calc_t settings_get_point_calculator(game_settings_t settings){
	if (settings & GAME_PERDERE_ENABLED)
		return card_points_perdere;
	else 
		return card_points_classic;
}


static inline void game_init(struct game_t *g, game_settings_t settings){
	// Load settings and init the game
	g->state = GAME_INIT;
	g->point_calculator = settings_get_point_calculator(settings);

	// scramble deck
	deck_init(g->deck);
	g->deck_len = DECK_SIZE;
	deck_scramble(g->deck, 1000); 

	g->table_len = 0;
	g->players_len = 0;
	g->current_player = 0;

	g->settings = settings;
}

static inline card_t _deck_draw(card_t *d, uint8_t *size){
	if (*size == 0) return END_OF_DECK;
	return d[--(*size)];
}

static inline void _game_init_hand(struct game_t *g, card_t *h) {
	card_t c;
	int8_t hand_size = game_hand_len(g);
	if (hand_size < 0){
		g->state = GAME_ERROR;
	}

	for (int i=0; i < hand_size; i++){
		c = DECK_DRAW(g->deck);

		if (c == END_OF_DECK)
			return;
		h[i] = c;
	}
}

static inline void game_start(struct game_t *g){
	if (g->state != GAME_INIT) 
		goto error;

	if (g->players_len <= 0)
		goto error;

	g->state = GAME_PLAYER_TURN;

	// deal the cards
	const uint8_t hand_len = game_hand_len(g);
	if (hand_len < 0)
		goto error;

	for (int i = 0; i < g->players_len; i++){
		_game_init_hand(g, g->players[i].hand);
		g->players[i].hand_len = hand_len;
	}

	if (g->players_len > 2)
		g->deck_len = 0;

	return;

error:
	g->state = GAME_ERROR;
	return;
}

#endif // TRESSETTE_H
