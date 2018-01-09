/*
* JA <-> Omni-Bot interface source file.
*
*/

#include <sstream>
#include <iomanip>

extern "C"
{
	#include "g_jabot_interface.h"
	#include "bg_saga.h"

	//qboolean G_IsOnFireteam( int entityNum, fireteamData_t** teamNum );
};

#include "BotExports.h"
#include "JA_Config.h"
#include "JA_Messages.h"

#define OMNIBOT_MIN_ENG 2
#define OMNIBOT_MIN_MED 2
#define OMNIBOT_MIN_FOP 2
#define OMNIBOT_MIN_SOL 2
#define OMNIBOT_MIN_COP 1

void Bot_EntityCreated( gentity_t * ent );

bool IsBot( gentity_t *e )
{
	return e->r.svFlags & SVF_BOT ? true : false;
}

//////////////////////////////////////////////////////////////////////////

//const int		MAX_SMOKEGREN_CACHE = 32;
//gentity_t		*g_SmokeGrenadeCache[ MAX_SMOKEGREN_CACHE ] = { 0 };

struct BotEntity
{
	int16_t	mHandleSerial;
	bool	mNewEntity : 1;
	bool	mUsed : 1;
};

BotEntity		mEntityHandles[ MAX_GENTITIES ];

//////////////////////////////////////////////////////////////////////////

// utils partly taken from id code
/*#define WC_WEAPON_TIME_LEFT level.time - ps->classWeaponTime
#define WC_SOLDIER_TIME		level.soldierChargeTime		[team-TEAM_AXIS]
#define WC_ENGINEER_TIME	level.engineerChargeTime	[team-TEAM_AXIS]
#define WC_FIELDOPS_TIME	level.lieutenantChargeTime	[team-TEAM_AXIS]
#define WC_MEDIC_TIME		level.medicChargeTime		[team-TEAM_AXIS]
#define WC_COVERTOPS_TIME	level.covertopsChargeTime	[team-TEAM_AXIS]*/

//////////////////////////////////////////////////////////////////////////

unsigned short	m_DeletedMapModels[ MAX_MODELS ] = {};
int				m_NumDeletedMapModels = 0;

gentity_t *INDEXENT( const int _gameId )
{
	if ( _gameId >= 0 && _gameId < MAX_GENTITIES )
	{
		switch ( _gameId )
		{
			case ENTITYNUM_WORLD: // world ent not marked inuse for some reason
				return &g_entities[ ENTITYNUM_WORLD ];
			default:
				return g_entities[ _gameId ].inuse ? &g_entities[ _gameId ] : 0;
		}
	}
	return 0;
}

int ENTINDEX( gentity_t *ent )
{
	return ent - g_entities;
}

gentity_t *EntityFromHandle( GameEntity ent )
{
	int16_t index = ent.GetIndex();
	if ( mEntityHandles[ index ].mHandleSerial == ent.GetSerial() && g_entities[ index ].inuse )
		return &g_entities[ index ];
	if ( index == ENTITYNUM_WORLD )
		return &g_entities[ ENTITYNUM_WORLD ];
	return NULL;
}

GameEntity HandleFromEntity( gentity_t *ent )
{
	if ( ent )
		return GameEntity( ent - g_entities, mEntityHandles[ ent - g_entities ].mHandleSerial );
	else
		return GameEntity();
}

//////////////////////////////////////////////////////////////////////////

bool GetOmnibotEntityType( gentity_t * ent, EntityInfo & classInfo );

//////////////////////////////////////////////////////////////////////////
enum
{
	MaxDeferredGoals = 64
};
MapGoalDef gDeferredGoals[ MaxDeferredGoals ];
int gNumDeferredGoals = 0;
bool gGoalSubmitReady = false;

void AddDeferredGoal( gentity_t *ent )
{
	if ( gNumDeferredGoals >= MaxDeferredGoals - 1 )
	{
		trap->Error( ERR_DROP, "Deferred Goal Buffer Full!" );
		return;
	}

	MapGoalDef &goaldef = gDeferredGoals[ gNumDeferredGoals++ ];
	MapGoalDef &goaldef2 = gDeferredGoals[ gNumDeferredGoals++ ];

	switch ( ent->s.eType )
	{
		/*case ET_MG42_BARREL:
		{
			goaldef.Props.SetString( "Type", "mountmg42" );
			goaldef.Props.SetEntity( "Entity", HandleFromEntity( ent ) );
			goaldef.Props.SetInt( "Team", ( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS ) );
			goaldef.Props.SetString( "TagName", _GetEntityName( ent ) );
			goaldef.Props.SetInt( "InterfaceGoal", 1 );

			// cs: this was done in et_goalmanager before
			goaldef2.Props.SetString( "Type", "repairmg42" );
			goaldef2.Props.SetEntity( "Entity", HandleFromEntity( ent ) );
			goaldef2.Props.SetInt( "Team", ( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS ) );
			goaldef2.Props.SetString( "TagName", _GetEntityName( ent ) );
			goaldef2.Props.SetInt( "InterfaceGoal", 1 );
			break;
		}*/
		default:
		{
			break;
		}
	}
}

void SendDeferredGoals()
{
	if ( gGoalSubmitReady )
	{
		for ( int i = 0; i < gNumDeferredGoals; ++i )
		{
			gBotFunctions->AddGoal( gDeferredGoals[ i ] );
		}
		gNumDeferredGoals = 0;
	}
}

void UpdateGoalEntity( gentity_t *oldent, gentity_t *newent )
{
	if ( gGoalSubmitReady )
	{
		gBotFunctions->UpdateEntity( HandleFromEntity( oldent ), HandleFromEntity( newent ) );
	}
}

void DeleteMapGoal( char *name )
{
	gBotFunctions->DeleteGoal( name );
}

#if 0
struct mg42s_t
{
	gentity_t *ent;
	vec3_t position;
	char name[ 64 ];
	char newname[ 64 ];
	bool buildable;
};

mg42s_t mg42s[ 64 ];
int numofmg42s = 0;
bool havemg42s = true;

void GetMG42s()
{
	if ( !numofmg42s && havemg42s )
	{
		gentity_t *trav = NULL;
		char *name;

		while ( ( trav = G_Find( trav, FOFS( classname ), "misc_mg42" ) ) )
		{
			name = (char *)_GetEntityName( trav );
			mg42s[ numofmg42s ].ent = trav;
			GetEntityCenter( trav, mg42s[ numofmg42s ].position );

			if ( name )
				strcpy( mg42s[ numofmg42s ].name, name );
			else
				mg42s[ numofmg42s ].name[ 0 ] = (char)NULL;

			mg42s[ numofmg42s ].buildable = false;
			numofmg42s++;
		}
	}

	if ( !numofmg42s )
		havemg42s = false;
}

void UpdateMG42( gentity_t *ent )
{
	vec3_t entpos;

	GetEntityCenter( ent, entpos );

	for ( int i = 0; i < numofmg42s; ++i )
	{
		if ( mg42s[ i ].position[ 0 ] == entpos[ 0 ] &&
			mg42s[ i ].position[ 1 ] == entpos[ 1 ] )
		{
			mg42s[ i ].ent = ent;
		}
	}
}

void CheckForMG42( gentity_t *ent, const char *newname )
{
	if ( !numofmg42s )
		return;

	vec3_t entpos;

	GetEntityCenter( ent, entpos );

	for ( int i = 0; i < numofmg42s; ++i )
	{
		if ( ( fabs( mg42s[ i ].position[ 0 ] - entpos[ 0 ] ) < 100.0 ) &&
			( fabs( mg42s[ i ].position[ 1 ] - entpos[ 1 ] ) < 100.0 ) )
		{
			mg42s[ i ].buildable = true;
			strcpy( mg42s[ i ].newname, newname );
		}
	}

	return;
}
#endif

void GetEntityCenter( gentity_t *ent, vec3_t _pos )
{
	_pos[ 0 ] = ent->r.currentOrigin[ 0 ] + ( ( ent->r.maxs[ 0 ] + ent->r.mins[ 0 ] ) * 0.5f );
	_pos[ 1 ] = ent->r.currentOrigin[ 1 ] + ( ( ent->r.maxs[ 1 ] + ent->r.mins[ 1 ] ) * 0.5f );
	_pos[ 2 ] = ent->r.currentOrigin[ 2 ] + ( ( ent->r.maxs[ 2 ] + ent->r.mins[ 2 ] ) * 0.5f );
}

//////////////////////////////////////////////////////////////////////////
const char *_GetEntityName( gentity_t *ent )
{
	// For goal names.
	//if(ent)
	//{
	//	if(ent->scriptName)
	//		return ent->scriptName;
	//	else if(ent->targetname)
	//		return ent->targetname;
	//	else if(ent->message)
	//		return ent->message;
	//}
	static char newentname[ 256 ];
	char *name;

	newentname[ 0 ] = '\0';

	if ( ent )
	{
		if ( ent->inuse && ent->client && !ent->NPC )
		{
			if ( ent->client->pers.netname[ 0 ] )
				return ent->client->pers.netname;
			else
			{
				static char	userinfo[ MAX_INFO_STRING ] = { 0 };
				trap->GetUserinfo( ent - g_entities, userinfo, sizeof( userinfo ) );
				return Info_ValueForKey( userinfo, "name" );
			}
		}

		//if ( ent->track )
		//	strcpy( newentname, ent->track );
		//else if ( ent->scriptName )
		//	strcpy( newentname, ent->scriptName );
		if(ent->item && ent->item->giType == IT_TEAM && ent->item->giTag == PW_REDFLAG && !(ent->flags & FL_DROPPED_ITEM))
			return "red_flag";
		else if(ent->item && ent->item->giType == IT_TEAM && ent->item->giTag == PW_BLUEFLAG && !(ent->flags & FL_DROPPED_ITEM))
			return "blue_flag";
		else if(ent->item && ent->item->giType == IT_TEAM && ent->item->giTag == PW_REDFLAG && (ent->flags & FL_DROPPED_ITEM))
			return "red_flag_dropped";
		else if(ent->item && ent->item->giType == IT_TEAM && ent->item->giTag == PW_BLUEFLAG && (ent->flags & FL_DROPPED_ITEM))
			return "blue_flag_dropped";
		else if(ent->classname && !Q_stricmp(ent->classname, "misc_siege_item") && ent->genericValue11)
			return OB_VA("siege_flag_%i", ent-g_entities);
		else if ( ent->script_targetname )
			strcpy( newentname, ent->script_targetname );
		else if ( ent->targetname )
			strcpy( newentname, ent->targetname );
		else if ( ent->message )
			strcpy( newentname, ent->message );

		name = newentname;

		Q_CleanStr( name );
		if ( name )
		{
			char undschar [] = { '-', (char)NULL };
			char skipchar [] = { '[', ']', '#', '!', '*', '`',
				'^', '&', '<', '>', '+', '=', '|', '\'', '%',
				'.', ':', '/', '(', ')', (char)NULL };
			char *curchar = NULL;
			char *tmp = NULL;
			char *tmpdst = NULL;
			tmp = name;
			tmpdst = name;

			while ( *tmp )
			{
				curchar = undschar;
				while ( *curchar )
				{
					if ( *tmp == *curchar )
					{
						*tmp = '_';
						break;
					}
					curchar++;
				}
				curchar = skipchar;
				while ( *curchar )
				{
					if ( *tmp == *curchar )
					{
						tmp++;
						break;
					}
					curchar++;
				}
				*tmpdst = *tmp;
				tmp++;
				tmpdst++;
			}

			*tmpdst = '\0';

			if ( !Q_stricmpn( "the ", name, 4 ) )
				return name + 4;

			return name;
		}
	}
	return NULL;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static bool weaponCharged( playerState_t* ps, team_t team, int weapon, int* skill )
{
	return qtrue;
}

// cs: added ignore ent so the bot wont count itself.
// this fixes soldier counts since latchPlayerType defaults to 0
static int CountPlayerClass( team_t team, int playerClass, int ignore )
{
	int num = 0;
	int j;

	for ( int i = 0; i < level.numConnectedClients; ++i )
	{
		if ( i == ignore )
			continue;

		j = level.sortedClients[ i ];

		if ( level.clients[ j ].sess.sessionTeam != team )
			continue;

		if ( level.clients[ j ].sess.latchPlayerType != playerClass )
			continue;
		num++;
	}
	return num;
}

int Bot_TeamGameToBot( int team )
{
	switch ( team )
	{
		case TEAM_RED:
			return JA_TEAM_RED;
		case TEAM_BLUE:
			return JA_TEAM_BLUE;
		case TEAM_FREE:
			return JA_TEAM_FREE;
		case TEAM_SPECTATOR:
			return OB_TEAM_SPECTATOR;
		default:
			return JA_TEAM_NONE;
	}
}

static int playerClassBotToGame( int playerClass )
{
	if ( g_gametype.integer != GT_SIEGE )
		return -2;

	switch ( playerClass )
	{
		case JA_CLASS_ASSAULT:
			return SPC_INFANTRY;
		case JA_CLASS_SCOUT:
			return SPC_VANGUARD;
		case JA_CLASS_TECH:
			return SPC_SUPPORT;
		case JA_CLASS_JEDI:
			return SPC_JEDI;
		case JA_CLASS_DEMO:
			return SPC_DEMOLITIONIST;
		case JA_CLASS_HW:
			return SPC_HEAVY_WEAPONS;
		default:
			return -1;
	}
}

int Bot_PlayerClassGameToBot( int playerClass )
{
	if ( g_gametype.integer != GT_SIEGE )
		return JA_CLASS_PLAYER;

	switch ( playerClass )
	{
		case SPC_INFANTRY:
			return JA_CLASS_ASSAULT;
		case SPC_VANGUARD:
			return JA_CLASS_SCOUT;
		case SPC_SUPPORT:
			return JA_CLASS_TECH;
		case SPC_JEDI:
			return JA_CLASS_JEDI;
		case SPC_DEMOLITIONIST:
			return JA_CLASS_DEMO;
		case SPC_HEAVY_WEAPONS:
			return JA_CLASS_HW;
		default:
			return JA_CLASS_NULL;
	}
}

static int _weaponBotToGame( int weapon )
{
	switch ( weapon )
	{
		case JA_WP_STUN_BATON:
			return WP_STUN_BATON;
		case JA_WP_MELEE:
			return WP_MELEE;
		case JA_WP_SABER:
			return WP_SABER;
		case JA_WP_BRYAR_PISTOL:
			return WP_BRYAR_PISTOL;
		case JA_WP_BLASTER:
			return WP_BLASTER;
		case JA_WP_DISRUPTOR:
			return WP_DISRUPTOR;
		case JA_WP_BOWCASTER:
			return WP_BOWCASTER;
		case JA_WP_REPEATER:
			return WP_REPEATER;
		case JA_WP_DEMP2:
			return WP_DEMP2;
		case JA_WP_FLECHETTE:
			return WP_FLECHETTE;
		case JA_WP_ROCKET_LAUNCHER:
			return WP_ROCKET_LAUNCHER;
		case JA_WP_THERMAL:
			return WP_THERMAL;
		case JA_WP_TRIP_MINE:
			return WP_TRIP_MINE;
		case JA_WP_DET_PACK:
			return WP_DET_PACK;
		case JA_WP_CONCUSSION:
			return WP_CONCUSSION;
		case JA_WP_BRYAR_OLD:
			return WP_BRYAR_OLD;
		case JA_WP_EMPLACED_GUN:
			return WP_EMPLACED_GUN;
		case JA_WP_TURRET:
			return WP_TURRET;
		default:
			return WP_NONE;
	}
}

int Bot_WeaponGameToBot( int weapon )
{
	switch ( weapon )
	{
		case WP_STUN_BATON:
			return JA_WP_STUN_BATON;
		case WP_MELEE:
			return JA_WP_MELEE;
		case WP_SABER:
			return JA_WP_SABER;
		case WP_BRYAR_PISTOL:
			return JA_WP_BRYAR_PISTOL;
		case WP_BLASTER:
			return JA_WP_BLASTER;
		case WP_DISRUPTOR:
			return JA_WP_DISRUPTOR;
		case WP_BOWCASTER:
			return JA_WP_BOWCASTER;
		case WP_REPEATER:
			return JA_WP_REPEATER;
		case WP_DEMP2:
			return JA_WP_DEMP2;
		case WP_FLECHETTE:
			return JA_WP_FLECHETTE;
		case WP_ROCKET_LAUNCHER:
			return JA_WP_ROCKET_LAUNCHER;
		case WP_THERMAL:
			return JA_WP_THERMAL;
		case WP_TRIP_MINE:
			return JA_WP_TRIP_MINE;
		case WP_DET_PACK:
			return JA_WP_DET_PACK;
		case WP_CONCUSSION:
			return JA_WP_CONCUSSION;
		case WP_BRYAR_OLD:
			return JA_WP_BRYAR_OLD;
		case WP_EMPLACED_GUN:
			return JA_WP_EMPLACED_GUN;
		case WP_TURRET:
			return JA_WP_TURRET;
		default:
			return JA_WP_NONE;
	}
}

#if 0
qboolean Simple_EmplacedGunIsRepairable( gentity_t* ent )
{
	if ( Q_stricmp( ent->classname, "misc_mg42" ) /*&& Q_stricmp( ent->classname, "misc_aagun" )*/ )
		return qfalse;
	if ( ent->s.frame == 0 )
		return qfalse;
	return qtrue;
}
#endif

void Bot_Util_AddGoal( const char *_type, gentity_t *ent, int _team, const char *_tag, const char *_extrakey = 0, obUserData *_extraval = 0 )
{
	if ( IsOmnibotLoaded() )
	{
		MapGoalDef goaldef;

		goaldef.Props.SetString( "Type", _type );
		goaldef.Props.SetEntity( "Entity", HandleFromEntity( ent ) );
		goaldef.Props.SetInt( "Team", _team );
		goaldef.Props.SetString( "TagName", _tag );
		goaldef.Props.SetInt( "InterfaceGoal", 1 );
		if ( _extrakey && _extraval )
		{
			goaldef.Props.Set( _extrakey, *_extraval );
		}
		gBotFunctions->AddGoal( goaldef );
	}
}

extern "C"
{
	extern qboolean G_EntIsBreakable(int entityNum);
};

static int _GetEntityTeam( gentity_t *ent )
{
	// hack, when the game joins clients again after warmup, they are temporarily ET_GENERAL entities(LAME)
	if ( ent->client && ( ent - g_entities ) < MAX_CLIENTS )
	{
		//t = ET_PLAYER;
		return Bot_TeamGameToBot( ent->client->sess.sessionTeam );
	}

	int t = ent->s.eType;

	switch ( t )
	{
		case ET_PLAYER:
			if ( g_gametype.integer == GT_SIEGE )
				return Bot_TeamGameToBot( ent->client->sess.siegeDesiredTeam );
			else
				return Bot_TeamGameToBot( ent->client->sess.sessionTeam );
#if 0
		case ET_CORPSE:
			return Bot_TeamGameToBot( BODY_TEAM( ent ) );
		case ET_MISSILE:
			if ( ent->s.weapon == WP_LANDMINE || ent->s.weapon == WP_DYNAMITE )
				return Bot_TeamGameToBot( G_LandmineTeam( ent ) );
			// Let this fall through
#endif
		case ET_NPC:
			return Bot_TeamGameToBot( ent->client->sess.sessionTeam );
		case ET_SPECIAL:
			return Bot_TeamGameToBot( ent->s.otherEntityNum2 );
		case ET_MISSILE:
			if( ent->parent && ent->parent->inuse && ent->parent->client )
				return Bot_TeamGameToBot( ent->parent->client->sess.sessionTeam );
			return 0;
		case ET_MOVER:
			if ( G_EntIsBreakable( ENTINDEX( ent ) ) )
				return Bot_TeamGameToBot( ent->teamnodmg );
		case ET_GENERAL:
		{
			if ( !Q_stricmp( ent->classname, "laserTrap" ) || !Q_stricmp( ent->classname, "misc_trip_mine" ) || !Q_stricmp( ent->classname, "detpack" ) ) {
				if ( ent->parent && ent->parent->inuse && ent->parent->client )
					return Bot_TeamGameToBot( ent->parent->client->sess.sessionTeam );
			}
			if ( !Q_stricmp( ent->classname, "misc_turret" ) ) {
				if ( ent->alliedTeam )
					return Bot_TeamGameToBot( ent->alliedTeam );

				if ( ent->teamnodmg )
					return Bot_TeamGameToBot( ent->teamnodmg );
			}
			if ( !Q_stricmp( ent->classname, "misc_turretG2" ) ) {
				if ( ent->alliedTeam )
					return Bot_TeamGameToBot( ent->alliedTeam );

				if ( ent->teamnodmg )
					return Bot_TeamGameToBot( ent->teamnodmg );
			}
			return 0;
		}
		default:
			return 0;
			//return Bot_TeamGameToBot( ent->s.teamNum );
	}

	return 0;
}

#if 0
qboolean _TankIsMountable( gentity_t *ent )
{
	if ( !( ent->spawnflags & 128 ) )
		return qfalse;

	if ( level.disableTankEnter )
		return qfalse;

	/*if( G_TankIsOccupied( ent ) )
	return qfalse;*/
	if ( omnibot_flags.integer & OBF_DONT_MOUNT_TANKS )
		return qfalse;

	if ( ent->health <= 0 )
		return qfalse;

	return qtrue;
}

qboolean _EmplacedGunIsMountable( gentity_t *ent )
{
	if ( omnibot_flags.integer & OBF_DONT_MOUNT_GUNS )
		return qfalse;
	return qtrue;
}
#endif

void Omnibot_Load_PrintMsg( const char *_msg )
{
	trap->Print( "Omni-bot: %s%s\n", S_COLOR_GREEN, _msg );
}

void Omnibot_Load_PrintErr( const char *_msg )
{
	trap->Print( "Omni-bot: %s%s\n", S_COLOR_RED, _msg );
}

int g_LastScreenMessageTime = 0;

inline void ConvertBit( int & srcValue, int & dstValue, int matchBit, int toBit )
{
	if ( srcValue & matchBit )
	{
		dstValue |= toBit;
		srcValue &= ~matchBit; // so we can debug bits we dont handle
	}
}

void Bot_Util_CheckForGoalEntity( GameEntity ent )
{
	if ( IsOmnibotLoaded() )
	{
		gentity_t *pEnt = EntityFromHandle( ent );

		if ( pEnt->inuse )
		{
			switch ( pEnt->s.eType )
			{
				case ET_ITEM:
				{
					char buffer[ 256 ] = { 0 };
					const char *pGoalName = _GetEntityName( pEnt );

					if ( !Q_stricmp( pEnt->classname, "team_CTF_redflag" ) )
					{
						// red flag
						if ( pEnt->s.otherEntityNum != -1 )
							pGoalName = _GetEntityName( &g_entities[ pEnt->s.otherEntityNum ] );
						sprintf( buffer, "%s_dropped", pGoalName ? pGoalName : "red_flag" );
						Bot_Util_AddGoal( "flag", pEnt, ( 1 << JA_TEAM_BLUE ), buffer );
						Bot_Util_AddGoal( "flagreturn", pEnt, ( 1 << JA_TEAM_RED ), buffer );
					}
					else if ( !Q_stricmp( pEnt->classname, "team_CTF_blueflag" ) )
					{
						// blue flag
						if ( pEnt->s.otherEntityNum != -1 )
							pGoalName = _GetEntityName( &g_entities[ pEnt->s.otherEntityNum ] );
						sprintf( buffer, "%s_dropped", pGoalName ? pGoalName : "blue_flag" );
						Bot_Util_AddGoal( "flag", pEnt, ( 1 << JA_TEAM_RED ), buffer );
						Bot_Util_AddGoal( "flagreturn", pEnt, ( 1 << JA_TEAM_BLUE ), buffer );
					}
					break;
				}
				default:
					break;
			}
		}
	}
}

// helper stuff
qboolean InFieldOfVision( vec3_t viewangles, float fov, vec3_t angles )
{
	float diff, angle;

	for ( int i = 0; i < 2; ++i )
	{
		angle = AngleMod( viewangles[ i ] );
		angles[ i ] = AngleMod( angles[ i ] );
		diff = angles[ i ] - angle;
		if ( angles[ i ] > angle )
		{
			if ( diff > 180.0 ) diff -= 360.0;
		}
		else
		{
			if ( diff < -180.0 ) diff += 360.0;
		}
		if ( diff > 0 )
		{
			if ( diff > fov * 0.5 ) return qfalse;
		}
		else
		{
			if ( diff < -fov * 0.5 ) return qfalse;
		}
	}
	return qtrue;
}

class JAInterface : public JediAcademy_Interface
{
public:
	int AddBot( const ParamsAddbot & data )
	{
		// wait until everything settles before adding bots on dedicated servers
		if ( dedicated.integer && ( level.time - level.startTime < 10000 ) )
		{
			return -1;
		}
		
		int num = trap->BotAllocateClient( );
		if ( num < 0 )
		{
			PrintError( "Could not add bot!" );
			PrintError( "No free slots!" );
			return -1;
		}

		char userinfo[ MAX_INFO_STRING ] = { 0 };

		std::stringstream guid;
		guid << "OMNIBOT" << std::setw( 2 ) << std::setfill( '0' ) << num << std::right << std::setw( 23 ) << "";

		gentity_t* bot = &g_entities[ num ];

		Info_SetValueForKey( userinfo, "name", data.mName );
		Info_SetValueForKey( userinfo, "rate", "25000" );
		Info_SetValueForKey( userinfo, "snaps", "20" );
		Info_SetValueForKey( userinfo, "ip", "localhost" );
		Info_SetValueForKey( userinfo, "ja_guid", guid.str().c_str() );
		//Info_SetValueForKey( userinfo, "cl_punkbuster", "0" );
		//Info_SetValueForKey( userinfo, "cl_anonymous", "0" );

		trap->SetUserinfo( num, userinfo );

		const char *s = 0;
		if ( ( s = ClientConnect( num, qtrue, qtrue ) ) != 0 )
		{
			PrintError( va( "Could not connect bot: %s", s ) );
			num = -1;
		}
		// bad hack to prevent unhandled errors being returned as successful connections
		return bot && bot->inuse ? num : -1;
	}

	void RemoveBot( const ParamsKickbot & data )
	{
		if ( data.mGameId != ParamsKickbot::InvalidGameId )
		{
			if ( data.mGameId >= 0 && data.mGameId < MAX_CLIENTS )
			{
				gentity_t *ent = &g_entities[ data.mGameId ];
				if ( IsBot( ent ) )
					trap->DropClient( data.mGameId, "disconnected" );
			}
		}
		else
		{
			char cleanNetName[ MAX_NETNAME ];
			char cleanName[ MAX_NAME_LENGTH ];

			Q_strncpyz( cleanName, data.mName, MAX_NAME_LENGTH );
			Q_CleanStr( cleanName );

			for ( int i = 0; i < level.maxclients; ++i )
			{
				if ( !g_entities[ i ].inuse )
					continue;
				if ( !IsBot( &g_entities[ i ] ) )
					continue;

				// clean stuff
				Q_strncpyz( cleanNetName, g_entities[ i ].client->pers.netname, MAX_NETNAME );
				Q_CleanStr( cleanNetName );

				if ( !Q_stricmp( cleanNetName, cleanName ) )
				{
					trap->DropClient( i, "disconnected" );
				}
			}
		}
	}

	obResult ChangeTeam( int client, int newteam, const Message * data )
	{
		char* teamName;

		gentity_t* bot = &g_entities[ client ];

		// find a team if we didn't get one and we need one ;-)
		if ( g_gametype.integer >= GT_TEAM && newteam != JA_TEAM_RED && newteam != JA_TEAM_BLUE )
		{
			if ( ( newteam == RANDOM_TEAM ) ||
				( bot->client->sess.sessionTeam != TEAM_RED &&
				bot->client->sess.sessionTeam != TEAM_BLUE ) )
			{
				if ( TeamCount( client, TEAM_RED ) <= TeamCount( client, TEAM_BLUE ) )
				{
					newteam = JA_TEAM_RED;
				}
				else
				{
					newteam = JA_TEAM_BLUE;
				}
			}
			else
			{
				// remember old team
				newteam = Bot_TeamGameToBot( bot->client->sess.sessionTeam );
			}
		}

		if ( newteam == JA_TEAM_RED )
		{
			teamName = "red";
		}
		else
		{
			teamName = "blue";
		}

		// always go to spectator first to solve problems on map restarts
		//SetTeam(bot, "spectator", qtrue, -1, -1, qfalse);
		{
			const int iBotTeam = Bot_TeamGameToBot( bot->client->sess.sessionTeam );
			const int iBotClass = Bot_PlayerClassGameToBot( bot->client->sess.latchPlayerType );

			//G_IsWeaponDisabled ?
			//if ( !bot->client->sess.latchPlayerWeapon || iBotTeam != newteam )
			//	bot->client->sess.latchPlayerWeapon = _weaponBotToGame( _choosePriWeap( bot, iBotClass, newteam ) );
			//if ( !bot->client->sess.latchPlayerWeapon2 || iBotTeam != newteam )
			//	bot->client->sess.latchPlayerWeapon2 = _weaponBotToGame( _chooseSecWeap( bot, iBotClass, newteam ) );
		}

		//if (!SetTeam(bot, teamName, qtrue, -1, -1, qfalse)) {
		//	PrintError("Bot could not join team!");
		//	return 0;
		//}

		// if SetTeam() fails, be sure to at least send a note to the bot about the current team
		// (else this won't be neccessary because on respawn messages will be sent automatically)
		//if ( !SetTeam( bot, teamName, qtrue, (weapon_t)-1, (weapon_t)-1, qfalse ) )
		SetTeam( bot, teamName );
		//{

		//}
		return Success;
	}

	obResult ChangeClass( int client, int newclass, const Message * data )
	{
		gentity_t* bot = &g_entities[ client ];

		if ( g_gametype.integer != GT_SIEGE )
			newclass = JA_CLASS_PLAYER;
			return Success;

		// find playerclass if we didn't got one
		if ( newclass <= JA_CLASS_NULL || newclass >= JA_CLASS_MAX )
		{
			if ( ( newclass == RANDOM_CLASS ) || ( bot->client->sess.latchPlayerType < 0 ) ||
				( bot->client->sess.latchPlayerType >= NUM_PLAYER_CLASSES ) )
			{
				team_t predictedTeam = bot->client->sess.sessionTeam; // desiredteam?

				if ( predictedTeam != TEAM_RED && predictedTeam != TEAM_BLUE )
				{
					if ( TeamCount( client, TEAM_RED ) <= TeamCount( client, TEAM_BLUE ) )
					{
						predictedTeam = TEAM_RED;
					}
					else
					{
						predictedTeam = TEAM_BLUE;
					}
				}

				// cs: make sure one of each if min is greater than zero
				int engineers = CountPlayerClass( predictedTeam, PC_ENGINEER, client );
				int medics = CountPlayerClass( predictedTeam, PC_MEDIC, client );
				int fieldops = CountPlayerClass( predictedTeam, PC_FIELDOPS, client );
				int soldiers = CountPlayerClass( predictedTeam, PC_SOLDIER, client );
				int covops = CountPlayerClass( predictedTeam, PC_COVERTOPS, client );

				if ( OMNIBOT_MIN_ENG > 0 && engineers == 0 )
				{
					newclass = JA_CLASS_ENGINEER;
				}
				else if ( OMNIBOT_MIN_MED > 0 && medics == 0 )
				{
					newclass = ET_CLASS_MEDIC;
				}
				else if ( OMNIBOT_MIN_FOP > 0 && fieldops == 0 )
				{
					newclass = ET_CLASS_FIELDOPS;
				}
				else if ( OMNIBOT_MIN_SOL > 0 && soldiers == 0 )
				{
					newclass = ET_CLASS_SOLDIER;
				}
				else if ( OMNIBOT_MIN_COP > 0 && covops == 0 )
				{
					newclass = ET_CLASS_COVERTOPS;
				}
				else if ( engineers < OMNIBOT_MIN_ENG )
				{
					newclass = ET_CLASS_ENGINEER;
				}
				else if ( medics < OMNIBOT_MIN_MED )
				{
					newclass = ET_CLASS_MEDIC;
				}
				else if ( fieldops < OMNIBOT_MIN_FOP )
				{
					newclass = ET_CLASS_FIELDOPS;
				}
				else if ( soldiers < OMNIBOT_MIN_SOL )
				{
					newclass = ET_CLASS_SOLDIER;
				}
				else if ( covops < OMNIBOT_MIN_COP )
				{
					newclass = ET_CLASS_COVERTOPS;
				}
				else
				{
					newclass = Bot_PlayerClassGameToBot( rand() % NUM_PLAYER_CLASSES );
				}

				// old stuff
				/*if (CountPlayerClass(predictedTeam, PC_ENGINEER) < OMNIBOT_MIN_ENG) {
				newclass = ET_CLASS_ENGINEER;
				} else if (CountPlayerClass(predictedTeam, PC_MEDIC) < OMNIBOT_MIN_MED) {
				newclass = ET_CLASS_MEDIC;
				} else if (CountPlayerClass(predictedTeam, PC_FIELDOPS) < OMNIBOT_MIN_FOP) {
				newclass = ET_CLASS_FIELDOPS;
				} else if (CountPlayerClass(predictedTeam, PC_SOLDIER) < OMNIBOT_MIN_SOL) {
				newclass = ET_CLASS_SOLDIER;
				} else if (CountPlayerClass(predictedTeam, PC_COVERTOPS) < OMNIBOT_MIN_COP) {
				newclass = ET_CLASS_COVERTOPS;
				} else {
				newclass = Bot_PlayerClassGameToBot(rand() % NUM_PLAYER_CLASSES);
				}*/
			}
			else
			{
				newclass = Bot_PlayerClassGameToBot( bot->client->sess.latchPlayerType );
			}
		}

		bot->client->sess.latchPlayerType = playerClassBotToGame( newclass );
#if(0)
		int team = Bot_TeamGameToBot( bot->client->sess.sessionTeam );
		Msg_PlayerChooseEquipment *pMsg = 0;
		if ( _data ) _data->Get2( pMsg );
		if ( pMsg )
		{
			if ( pMsg->mWeaponChoice[ 0 ] )
				bot->client->sess.latchPlayerWeapon = _weaponBotToGame( pMsg->mWeaponChoice[ 0 ] );
			if ( pMsg->mWeaponChoice[ 1 ] )
				bot->client->sess.latchPlayerWeapon2 = _weaponBotToGame( pMsg->mWeaponChoice[ 1 ] );

#ifdef NOQUARTER
			if(G_IsWeaponDisabled(bot,(weapon_t)bot->client->sess.latchPlayerWeapon,qtrue))
				bot->client->sess.latchPlayerWeapon = 0;
			if(G_IsWeaponDisabled(bot,(weapon_t)bot->client->sess.latchPlayerWeapon2,qtrue))
				bot->client->sess.latchPlayerWeapon2 = 0;
#elif defined(ETPUB_VERSION)
			if(G_IsWeaponDisabled(bot,(weapon_t)bot->client->sess.latchPlayerWeapon,bot->client->sess.sessionTeam,qtrue))
				bot->client->sess.latchPlayerWeapon = 0;
			if(G_IsWeaponDisabled(bot,(weapon_t)bot->client->sess.latchPlayerWeapon2,bot->client->sess.sessionTeam,qtrue))
				bot->client->sess.latchPlayerWeapon2 = 0;
#else
			if ( G_IsWeaponDisabled( bot, (weapon_t)bot->client->sess.latchPlayerWeapon ) )
				bot->client->sess.latchPlayerWeapon = 0;
			if ( G_IsWeaponDisabled( bot, (weapon_t)bot->client->sess.latchPlayerWeapon2 ) )
				bot->client->sess.latchPlayerWeapon2 = 0;
#endif
		}

		{
			const int iBotClass = Bot_PlayerClassGameToBot( bot->client->sess.latchPlayerType );
			if ( !bot->client->sess.latchPlayerWeapon || bot->client->sess.latchPlayerType != bot->client->sess.playerType )
				bot->client->sess.latchPlayerWeapon = _weaponBotToGame( _choosePriWeap( bot, iBotClass, team ) );
			if ( !bot->client->sess.latchPlayerWeapon2 || bot->client->sess.latchPlayerType != bot->client->sess.playerType )
				bot->client->sess.latchPlayerWeapon2 = _weaponBotToGame( _chooseSecWeap( bot, iBotClass, team ) );
		}
#endif
		// commit suicide to ensure new class is used
		// CS: wait until 2 seconds before next spawn
		if ( bot->client->sess.latchPlayerType != bot->client->sess.playerType )
		{
			//round end.
			if ( bot->client->ps.pm_flags & PMF_TIME_LOCKPLAYER )
			{
				Cmd_Kill_f( bot );
			}
			else if ( !( bot->client->ps.pm_flags & PMF_LIMBO ) )
			{
				bot->client->sess.botSuicide = qtrue;
			}
		}
		return Success;
	}

	bool DebugLine( const float _start[ 3 ], const float _end[ 3 ], const obColor &_color, float _time )
	{
		// for dedicated servers we tell the bot we can handle this function, so it doesn't open
		// an IPC channel.
		if ( dedicated.integer )
			return true;

		return false;
	}

	bool DebugRadius( const float _pos[ 3 ], const float _radius, const obColor &_color, float _time )
	{
		// for dedicated servers we tell the bot we can handle this function, so it doesn't open
		// an IPC channel.
		if ( dedicated.integer )
			return true;

		return false;
	}

	void UpdateBotInput( int _client, const ClientInput &_input )
	{
		static usercmd_t cmd;
		gentity_t *bot = &g_entities[ _client ];

		// only causes problems
		bot->client->ps.pm_flags &= ~PMF_RESPAWNED;

		memset( &cmd, 0, sizeof( cmd ) );

		cmd.serverTime = level.time;

		// Set the weapon
		cmd.weapon = _weaponBotToGame( _input.mCurrentWeapon );

		// Set the currently selected force power
		cmd.forcesel = bot->client->ps.fd.forcePowerSelected;

		// Process the bot keypresses.
		if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_RESPAWN ) )
			cmd.buttons |= BUTTON_ATTACK;
		if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_ATTACK1 ) )
			cmd.buttons |= BUTTON_ATTACK;
		if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_ATTACK2 ) )
			cmd.buttons |= BUTTON_ALT_ATTACK;
		if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_WALK ) )
			cmd.buttons |= BUTTON_WALKING;
		if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_USE ) )
		{
			cmd.buttons |= BUTTON_USE_HOLDABLE;
			cmd.buttons |= BUTTON_USE;
		}

		if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_FORCEPOWER ) )
			cmd.buttons |= BUTTON_FORCEPOWER;
		if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_FORCEGRIP ) )
			cmd.buttons |= BUTTON_FORCEGRIP;
		if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_FORCELIGHTNING ) )
			cmd.buttons |= BUTTON_FORCE_LIGHTNING;
		if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_FORCEDRAIN ) )
			cmd.buttons |= BUTTON_FORCE_DRAIN;

		// don't process view angles and moving stuff when dead
		if ( bot->client->ps.pm_type >= PM_DEAD || bot->client->tempSpectate >= level.time || ( bot->client->ps.eFlags2 & EF2_HELD_BY_MONSTER ) )
		{
			// cant move in these states
			cmd.buttons &= ~BUTTON_ATTACK;
			cmd.buttons &= ~BUTTON_ALT_ATTACK;
		}
		else
		{
			float fMaxSpeed = 127.f;
			vec3_t eyeangles, bodyangles, forward, right;

			// Convert the bots vector to angles and set the view angle to the orientation
			vectoangles( _input.mFacing, eyeangles );
			SetClientViewAngle( bot, eyeangles );

			if ( cmd.buttons & BUTTON_WALKING )
				fMaxSpeed = 64.f;

			// Convert the move direction into forward and right moves to
			// take the bots orientation into account.

			// flatten the view angles so we get a proper fwd,right vector as relevent to movement.
			VectorCopy( eyeangles, bodyangles );
			bodyangles[ PITCH ] = 0;

			AngleVectors( bodyangles, forward, right, NULL );
			const float fwd = DotProduct( forward, _input.mMoveDir );
			const float rght = DotProduct( right, _input.mMoveDir );

			cmd.forwardmove = (char)( fwd * fMaxSpeed );
			cmd.rightmove = (char)( rght * fMaxSpeed );

			if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_FWD ) || _input.mButtonFlags.CheckFlag( BOT_BUTTON_MOVEUP ) )
				cmd.forwardmove = (char)fMaxSpeed;
			if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_BACK ) || _input.mButtonFlags.CheckFlag( BOT_BUTTON_MOVEDN ) )
				cmd.forwardmove = (char)-fMaxSpeed;
			if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_RSTRAFE ) )
				cmd.rightmove = (char)fMaxSpeed;
			if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_LSTRAFE ) )
				cmd.rightmove = (char)-fMaxSpeed;
			if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_JUMP ) )
				cmd.upmove = (char)fMaxSpeed;
			if ( _input.mButtonFlags.CheckFlag( BOT_BUTTON_CROUCH ) )
				cmd.upmove = (char)-fMaxSpeed;
		}
		trap->BotUserCommand( _client, &cmd );
	}

	void BotCommand( int _client, const char *_cmd )
	{
		trap->EA_Command( _client, (char *)_cmd );
	}

	bool IsInPVS( const float _pos[ 3 ], const float _target[ 3 ] )
	{
		return trap->InPVS( _pos, _target ) == qtrue;
	}

	obResult TraceLine( obTraceResult &_result, const float _start[ 3 ], const float _end[ 3 ],
		const AABB *_pBBox, int _mask, int _user, bool _bUsePVS )
	{
		qboolean bInPVS = _bUsePVS ? trap->InPVS( _start, _end ) : qtrue;
		if ( bInPVS )
		{
			int iMask = 0;

			// Set up the collision masks
			if ( _mask & TR_MASK_ALL )
				iMask = MASK_ALL & ~TR_MASK_FLOODFILL;
			else
			{
				if ( _mask & TR_MASK_SOLID )
					iMask |= MASK_SOLID;
				if ( _mask & TR_MASK_PLAYER )
					iMask |= MASK_PLAYERSOLID;
				if ( _mask & TR_MASK_SHOT )
					iMask |= MASK_SHOT;
				if ( _mask & TR_MASK_OPAQUE )
					iMask |= MASK_OPAQUE;
				if ( _mask & TR_MASK_WATER )
					iMask |= MASK_WATER;
				if ( _mask & TR_MASK_PLAYERCLIP )
					iMask |= CONTENTS_PLAYERCLIP;
				if ( _mask & ( TR_MASK_FLOODFILL | TR_MASK_FLOODFILLENT ) )
					iMask |= CONTENTS_PLAYERCLIP | CONTENTS_SOLID;
			}

			trace_t tr;
			tr.surfaceFlags = 0;

			if ( _mask & TR_MASK_FLOODFILL )
			{
				trap->Trace/*NoEnts*/( &tr, _start,
					_pBBox ? _pBBox->mMins : NULL,
					_pBBox ? _pBBox->mMaxs : NULL,
					_end, _user, iMask );
			}
			else
			{
				trap->Trace( &tr, _start,
					_pBBox ? _pBBox->mMins : NULL,
					_pBBox ? _pBBox->mMaxs : NULL,
					_end, _user, iMask );
			}

			if ( ( tr.entityNum != ENTITYNUM_WORLD ) && ( tr.entityNum != ENTITYNUM_NONE ) )
				_result.mHitEntity = HandleFromEntity( &g_entities[ tr.entityNum ] );
			else
				_result.mHitEntity.Reset();

			//_result.miUser1 = tr.surfaceFlags;

			// Fill in the bot traceflag.
			_result.mFraction = tr.fraction;
			_result.mStartSolid = tr.startsolid;
			_result.mEndpos[ 0 ] = tr.endpos[ 0 ];
			_result.mEndpos[ 1 ] = tr.endpos[ 1 ];
			_result.mEndpos[ 2 ] = tr.endpos[ 2 ];
			_result.mNormal[ 0 ] = tr.plane.normal[ 0 ];
			_result.mNormal[ 1 ] = tr.plane.normal[ 1 ];
			_result.mNormal[ 2 ] = tr.plane.normal[ 2 ];
			_result.mContents = ConvertValue( tr.contents, ConvertContentsFlags, ConvertGameToBot );
			_result.mSurface = ConvertValue( tr.surfaceFlags, ConvertSurfaceFlags, ConvertGameToBot );
		}
		else
		{
			// Not in PVS
			_result.mFraction = 0.0f;
			_result.mHitEntity.Reset();
		}
		return bInPVS ? Success : OutOfPVS;
	}

	virtual int GetPointContents( const float _pos[ 3 ] )
	{
		vec3_t vpos = { _pos[ 0 ], _pos[ 1 ], _pos[ 2 ] };
		const int iContents = trap->PointContents( vpos, -1 );
		return ConvertValue( iContents, ConvertContentsFlags, ConvertGameToBot );
	}

	virtual int ConvertValue( int value, ConvertType ctype, ConvertDirection cdir )
	{
		if ( cdir == ConvertGameToBot )
		{
			switch ( ctype )
			{
				case ConvertSurfaceFlags:
				{
					// clear flags we don't care about
					value &= ~( SURF_NOIMPACT | SURF_NOMARKS |
						SURF_SPLASH | SURF_HINT |
						SURF_NOLIGHTMAP | SURF_POINTLIGHT |
						SURF_LIGHTFILTER | SURF_ALPHASHADOW |
						SURF_NODLIGHT | SURF_METAL | SURF_WOOD |
						SURF_GRASS | SURF_GRAVEL |
						SURF_GLASS | SURF_SNOW |
						SURF_ROOF | SURF_RUBBLE | SURF_CARPET );

					int iBotSurface = 0;
					ConvertBit( value, iBotSurface, SURF_NONSOLID, SURFACE_NONSOLID );
					ConvertBit( value, iBotSurface, SURF_NOSTEPS, SURFACE_NOFOOTSTEP );
					ConvertBit( value, iBotSurface, SURF_SKY, SURFACE_SKY );
					ConvertBit( value, iBotSurface, SURF_SKIP, SURFACE_IGNORE );
					ConvertBit( value, iBotSurface, SURF_NODAMAGE, SURFACE_NOFALLDAMAGE );
					ConvertBit( value, iBotSurface, SURF_SLICK, SURFACE_SLICK );
					ConvertBit( value, iBotSurface, SURF_LADDER, SURFACE_LADDER );
					ConvertBit( value, iBotSurface, SURF_NODRAW, SURFACE_NODRAW );
					//ConvertBit( value, iBotSurface, SURF_LANDMINE, SURFACE_MINE );

					assert( value == 0 && "Unhandled flag" );
					return iBotSurface;
				}
				case ConvertContentsFlags:
				{
					const int CLIPSHOT_I_THINK = 0x00002000;

					value &= ~( CONTENTS_LIGHTGRID | CONTENTS_MISSILECLIP |
						CONTENTS_ITEM | CONTENTS_AREAPORTAL | CONTENTS_MONSTERCLIP |
						CONTENTS_CLUSTERPORTAL | CONTENTS_DONOTENTER |
						CONTENTS_DONOTENTER_LARGE | CONTENTS_ORIGIN |
						CONTENTS_STRUCTURAL | CONTENTS_TRANSLUCENT |
						CONTENTS_NODROP | CONTENTS_BODY | CONTENTS_CORPSE |
						CLIPSHOT_I_THINK );

					int iBotContents = 0;
					ConvertBit( value, iBotContents, CONTENTS_SOLID, CONT_SOLID );
					ConvertBit( value, iBotContents, CONTENTS_WATER, CONT_WATER );
					ConvertBit( value, iBotContents, CONTENTS_SLIME, CONT_SLIME );
					ConvertBit( value, iBotContents, CONTENTS_FOG, CONT_FOG );
					ConvertBit( value, iBotContents, CONTENTS_TELEPORTER, CONT_TELEPORTER );
					ConvertBit( value, iBotContents, CONTENTS_JUMPPAD, CONT_JUMPPAD );
					ConvertBit( value, iBotContents, CONTENTS_MOVER, CONT_MOVER );
					ConvertBit( value, iBotContents, CONTENTS_TRIGGER, CONT_TRIGGER );
					ConvertBit( value, iBotContents, CONTENTS_LAVA, CONT_LAVA );
					ConvertBit( value, iBotContents, CONTENTS_PLAYERCLIP, CONT_PLYRCLIP );
					ConvertBit( value, iBotContents, CONTENTS_DETAIL, CONT_NONSOLID );
					ConvertBit( value, iBotContents, CONTENTS_FOG, CONT_FOG );

					assert( value == 0 && "Unhandled flag" );
					return iBotContents;
				}
			}
		}
		else
		{
			switch ( ctype )
			{
				case ConvertSurfaceFlags:
				{
					int iBotSurface = 0;
					ConvertBit( value, iBotSurface, SURFACE_NONSOLID, SURF_NONSOLID );
					ConvertBit( value, iBotSurface, SURFACE_NOFOOTSTEP, SURF_NOSTEPS );
					ConvertBit( value, iBotSurface, SURFACE_SKY, SURF_SKY );
					ConvertBit( value, iBotSurface, SURFACE_IGNORE, SURF_SKIP );
					ConvertBit( value, iBotSurface, SURFACE_NOFALLDAMAGE, SURF_NODAMAGE );
					ConvertBit( value, iBotSurface, SURFACE_SLICK, SURF_SLICK );
					ConvertBit( value, iBotSurface, SURFACE_LADDER, SURF_LADDER );
					ConvertBit( value, iBotSurface, SURFACE_MINE, SURF_LANDMINE );

					assert( value == 0 && "Unhandled flag" );
					return iBotSurface;
				}
				case ConvertContentsFlags:
				{
					int iBotContents = 0;
					ConvertBit( value, iBotContents, CONT_SOLID, CONTENTS_SOLID );
					ConvertBit( value, iBotContents, CONT_WATER, CONTENTS_WATER );
					ConvertBit( value, iBotContents, CONT_SLIME, CONTENTS_SLIME );
					ConvertBit( value, iBotContents, CONT_FOG, CONTENTS_FOG );
					ConvertBit( value, iBotContents, CONT_TELEPORTER, CONTENTS_TELEPORTER );
					ConvertBit( value, iBotContents, CONT_JUMPPAD, CONTENTS_JUMPPAD );
					ConvertBit( value, iBotContents, CONT_MOVER, CONTENTS_MOVER );
					ConvertBit( value, iBotContents, CONT_TRIGGER, CONTENTS_TRIGGER );
					ConvertBit( value, iBotContents, CONT_LAVA, CONTENTS_LAVA );
					ConvertBit( value, iBotContents, CONT_PLYRCLIP, CONTENTS_PLAYERCLIP );
					ConvertBit( value, iBotContents, CONT_NONSOLID, CONTENTS_DETAIL );
					ConvertBit( value, iBotContents, CONT_FOG, CONTENTS_FOG );

					assert( value == 0 && "Unhandled flag" );
					return iBotContents;
				}
			}
		}
		assert( 0 && "Unhandled conversion" );
		return 0;
	}

	GameEntity GetLocalGameEntity()
	{
		return EntityFromID( 0 );
	}

	obResult GetEntityName( const GameEntity ent, obStringBuffer& nameOut )
	{
		const char* entityName = _GetEntityName( EntityFromHandle( ent ) );
		if ( entityName != NULL )
		{
			strncpy( nameOut.mBuffer, entityName, obStringBuffer::BUFFER_LENGTH );
			return Success;
		}
		return InvalidEntity;
	}

	obResult GetEntityForName( const char* entName, GameEntity& entityOut )
	{
		obStringBuffer str;
		for ( int i = 0; i < MAX_GENTITIES; ++i )
		{
			if ( g_entities[ i ].inuse )
			{
				if ( GetEntityName( HandleFromEntity( &g_entities[ i ] ), str ) == Success )
				{
					if ( !_stricmp( entName, str.mBuffer ) )
					{
						entityOut = HandleFromEntity( &g_entities[ i ] );
						return Success;
					}
				}
			}
		}
		return InvalidEntity;
	}

	obResult GetEntityInfo( const GameEntity gameEnt, EntityInfo& classInfo )
	{
		gentity_t * ent = EntityFromHandle( gameEnt );
		if ( ent )
		{
			classInfo = EntityInfo();
			if ( GetOmnibotEntityType( ent, classInfo ) )
				return Success;
		}
		return InvalidEntity;
	}

	obResult GetEntityEyePosition( const GameEntity gameEnt, float _pos[ 3 ] )
	{
		if ( GetEntityPosition( gameEnt, _pos ) == Success )
		{
			gentity_t *pEnt = EntityFromHandle( gameEnt );
			if ( pEnt && pEnt->client )
			{
				_pos[ 2 ] += pEnt->client->ps.viewheight;
			}
			return Success;
		}
		return InvalidEntity;
	}

	obResult GetEntityBonePosition( const GameEntity ent, int _boneid, float _pos[ 3 ] )
	{
		// ET doesnt really support bones
		return GetEntityPosition( ent, _pos );
	}

	obResult GetEntityOrientation( const GameEntity ent, float _fwd[ 3 ], float _right[ 3 ], float _up[ 3 ] )
	{
		gentity_t *pEnt = EntityFromHandle( ent );
		if ( pEnt && pEnt->inuse )
		{
			if ( pEnt->client )
			{
				AngleVectors( pEnt->client->ps.viewangles, _fwd, _right, _up );
			}
			else
			{
				AngleVectors( pEnt->r.currentAngles, _fwd, _right, _up ); //s.angles
			}
			return Success;
		}
		return InvalidEntity;
	}

	obResult GetEntityVelocity( const GameEntity ent, float _velocity[ 3 ] )
	{
		gentity_t *pEnt = EntityFromHandle( ent );
		if ( pEnt && pEnt->inuse )
		{
			// hack, when the game joins clients again after warmup, they are temporarily ET_GENERAL entities(LAME)
			int t = pEnt->s.eType;
			if ( pEnt->client && ( pEnt - g_entities ) < MAX_CLIENTS )
				t = ET_PLAYER;

			if ( t == ET_MOVER )
			{
				// Calculate the velocity ourselves. for some reason mover velocity
				// isn't in s.pos.trDelta
				const float fDeltaTime = 50.0f; // get this dynamically?
				_velocity[ 0 ] = ( pEnt->r.currentOrigin[ 0 ] - pEnt->oldOrigin[ 0 ] ) * fDeltaTime;
				_velocity[ 1 ] = ( pEnt->r.currentOrigin[ 1 ] - pEnt->oldOrigin[ 1 ] ) * fDeltaTime;
				_velocity[ 2 ] = ( pEnt->r.currentOrigin[ 2 ] - pEnt->oldOrigin[ 2 ] ) * fDeltaTime;
			}
			else
			{
				_velocity[ 0 ] = pEnt->s.pos.trDelta[ 0 ];
				_velocity[ 1 ] = pEnt->s.pos.trDelta[ 1 ];
				_velocity[ 2 ] = pEnt->s.pos.trDelta[ 2 ];
			}
			return Success;
		}
		return InvalidEntity;
	}

	obResult GetEntityPosition( const GameEntity ent, float _pos[ 3 ] )
	{
		gentity_t *pEnt = EntityFromHandle( ent );
		if ( pEnt && pEnt->inuse )
		{
			if ( !pEnt->client )
			{
				VectorCopy( pEnt->r.currentOrigin, _pos );

				//vec3_t axis[3];
				////AngleVectors( pEnt->r.currentAngles, axis[0], axis[1], axis[2] );
				//AnglesToAxis( pEnt->r.currentAngles, axis );

				//vec3_t boxCenter;
				//boxCenter[0] = ((pEnt->r.maxs[0] + pEnt->r.mins[0]) * 0.5f);
				//boxCenter[1] = ((pEnt->r.maxs[1] + pEnt->r.mins[1]) * 0.5f);
				//boxCenter[2] = ((pEnt->r.maxs[2] + pEnt->r.mins[2]) * 0.5f);

				//vec3_t out;
				//VectorCopy(pEnt->r.currentOrigin,out);
				//for( int i = 0; i < 3; ++i ) {
				//	vec3_t tmp;
				//	VectorScale( axis[i], boxCenter[i], tmp );
				//	VectorAdd( out, tmp, out );
				//}
				//VectorCopy( out, _pos );
				return Success;
			}

			// Clients return normal position.
			if ( pEnt->client->sess.sessionTeam == TEAM_SPECTATOR )
				VectorCopy( pEnt->s.origin, _pos );
			else
				VectorCopy( pEnt->r.currentOrigin, _pos );

			return Success;
		}
		return InvalidEntity;
	}

	obResult GetEntityLocalAABB( const GameEntity ent, AABB &_aabb )
	{
		gentity_t *pEnt = EntityFromHandle( ent );
		if ( pEnt && pEnt->inuse )
		{
			/*if(!pEnt->r.linked && pEnt->parent && pEnt->parent->s.eType == ET_OID_TRIGGER)
			pEnt = pEnt->parent;*/

			//if ( pEnt->s.eType == ET_CONSTRUCTIBLE )
			//{
			//	gentity_t *pAxis = G_ConstructionForTeam( pEnt->parent ? pEnt->parent : pEnt, TEAM_AXIS );
			//	gentity_t *pAlly = G_ConstructionForTeam( pEnt->parent ? pEnt->parent : pEnt, TEAM_ALLIES );
			//	if ( pAxis /*&& pAxis->entstate == STATE_DEFAULT*/ )
			//		pEnt = pAxis;
			//	else if ( pAlly /*&& pAlly->entstate == STATE_DEFAULT*/ )
			//		pEnt = pAlly;
			//}

			//if(!Q_stricmp(pEnt->classname, "func_explosive") ||
			//	!Q_stricmpn(pEnt->classname, "trigger_",8))
			//{
			//	// find the midpt
			//	vec3_t pos;
			//	pos[0] = pEnt->r.currentOrigin[0] + ((pEnt->r.maxs[0] + pEnt->r.mins[0]) * 0.5f);
			//	pos[1] = pEnt->r.currentOrigin[1] + ((pEnt->r.maxs[1] + pEnt->r.mins[1]) * 0.5f);
			//	pos[2] = pEnt->r.currentOrigin[2] + ((pEnt->r.maxs[2] + pEnt->r.mins[2]) * 0.5f);

			//	// figure out the local bounds from there
			//	_aabb.mMins[0] = pEnt->r.mins[0] - pos[0];
			//	_aabb.mMins[1] = pEnt->r.mins[1] - pos[1];
			//	_aabb.mMins[2] = pEnt->r.mins[2] - pos[2];
			//	_aabb.mMaxs[0] = pEnt->r.maxs[0] - pos[0];
			//	_aabb.mMaxs[1] = pEnt->r.maxs[1] - pos[1];
			//	_aabb.mMaxs[2] = pEnt->r.maxs[2] - pos[2];
			//	return Success;
			//}

			//if(!Q_stricmp(pEnt->classname, "script_mover"))
			//{
			//	vec3_t fwd,rgt,up,mins,maxs;
			//	AngleVectors(pEnt->r.currentAngles, fwd, rgt, up);
			//	VectorCopy(pEnt->r.currentOrigin,mins);
			//	VectorCopy(pEnt->r.currentOrigin,maxs);

			//	VectorSubtract(mins,x,mins);

			//	VectorAdd();

			//	// start at the origin
			//	_aabb.mMins[0] = _aabb.mMaxs[0] = pEnt->r.currentOrigin[0];
			//	_aabb.mMins[0] = _aabb.mMaxs[0] = pEnt->r.currentOrigin[0];
			//	_aabb.mMins[0] = _aabb.mMaxs[0] = pEnt->r.currentOrigin[0];

			//	// find the mins/maxs
			//	_aabb.mMins[0] = pEnt->r.currentOrigin[0] + pEnt->r.mins[0] * fwd[0];
			//	_aabb.mMins[1] = pEnt->r.currentOrigin[1] + pEnt->r.mins[1] * fwd[1];
			//	_aabb.mMins[1] = pEnt->r.currentOrigin[1] + pEnt->r.mins[2] * fwd[2];

			//	_aabb.mMins[0] = pEnt->r.mins[0] * fwd;
			//	_aabb.mMins[1] = pEnt->r.mins[1] - pos[1];
			//	_aabb.mMins[2] = pEnt->r.mins[2] - pos[2];
			//	_aabb.mMaxs[0] = pEnt->r.maxs[0] - pos[0];
			//	_aabb.mMaxs[1] = pEnt->r.maxs[1] - pos[1];
			//	_aabb.mMaxs[2] = pEnt->r.maxs[2] - pos[2];

			//	return Success;
			//}

			_aabb.mMins[ 0 ] = pEnt->r.mins[ 0 ];
			_aabb.mMins[ 1 ] = pEnt->r.mins[ 1 ];
			_aabb.mMins[ 2 ] = pEnt->r.mins[ 2 ];
			_aabb.mMaxs[ 0 ] = pEnt->r.maxs[ 0 ];
			_aabb.mMaxs[ 1 ] = pEnt->r.maxs[ 1 ];
			_aabb.mMaxs[ 2 ] = pEnt->r.maxs[ 2 ];

			// hack for bad abs bounds
			if ( !Q_stricmp( pEnt->classname, "misc_mg42" ) )
			{
				if ( _aabb.IsZero() )
				{
					const float default_box_mins [] = { -8, -8, -8 };
					const float default_box_maxs [] = { 8, 8, 48 };
					_aabb.Set( default_box_mins, default_box_maxs );
				}
			}
			return Success;
		}
		return InvalidEntity;
	}
	obResult GetEntityWorldAABB( const GameEntity ent, AABB &_aabb )
	{
		gentity_t *pEnt = EntityFromHandle( ent );
		if ( pEnt && pEnt->inuse )
		{
			/*if(!pEnt->r.linked && pEnt->parent && pEnt->parent->s.eType == ET_OID_TRIGGER)
			pEnt = pEnt->parent;*/

			//if ( pEnt->s.eType == ET_CONSTRUCTIBLE )
			//{
			//	gentity_t *pAxis = G_ConstructionForTeam( pEnt->parent ? pEnt->parent : pEnt, TEAM_AXIS );
			//	gentity_t *pAlly = G_ConstructionForTeam( pEnt->parent ? pEnt->parent : pEnt, TEAM_ALLIES );
			//	if ( pAxis /*&& pAxis->entstate == STATE_DEFAULT*/ )
			//		pEnt = pAxis;
			//	else if ( pAlly /*&& pAlly->entstate == STATE_DEFAULT*/ )
			//		pEnt = pAlly;
			//}

			_aabb.mMins[ 0 ] = pEnt->r.absmin[ 0 ];
			_aabb.mMins[ 1 ] = pEnt->r.absmin[ 1 ];
			_aabb.mMins[ 2 ] = pEnt->r.absmin[ 2 ];
			_aabb.mMaxs[ 0 ] = pEnt->r.absmax[ 0 ];
			_aabb.mMaxs[ 1 ] = pEnt->r.absmax[ 1 ];
			_aabb.mMaxs[ 2 ] = pEnt->r.absmax[ 2 ];

			// raise player bounds slightly since it appears to be in the ground a bit
			if ( pEnt->client )
			{
				_aabb.mMins[ 2 ] += 2.f;
				_aabb.mMaxs[ 2 ] += 2.f;
			}

			// hack for bad abs bounds
			if ( !Q_stricmp( pEnt->classname, "misc_mg42" ) )
			{
				if ( _aabb.IsZero() || !_aabb.Contains( pEnt->r.currentOrigin ) )
				{
					float pos[ 3 ] = { 0, 0, 0 };
					const float default_box_mins [] = { -8, -8, -8 };
					const float default_box_maxs [] = { 8, 8, 48 };
					GetEntityPosition( ent, pos );
					_aabb.Set( default_box_mins, default_box_maxs );
					_aabb.SetCenter( pos );
				}
			}
			return Success;
		}
		return InvalidEntity;
	}

	obResult GetEntityWorldOBB( const GameEntity ent, float *_center, float *_axis0, float *_axis1, float *_axis2, float *_extents )
	{
		gentity_t *pEnt = EntityFromHandle( ent );
		if ( pEnt )
		{
			vec3_t axis[ 3 ];
			AnglesToAxis( pEnt->r.currentAngles, axis );

			vec3_t boxCenter;
			boxCenter[ 0 ] = ( ( pEnt->r.maxs[ 0 ] + pEnt->r.mins[ 0 ] ) * 0.5f );
			boxCenter[ 1 ] = ( ( pEnt->r.maxs[ 1 ] + pEnt->r.mins[ 1 ] ) * 0.5f );
			boxCenter[ 2 ] = ( ( pEnt->r.maxs[ 2 ] + pEnt->r.mins[ 2 ] ) * 0.5f );

			vec3_t out;
			VectorCopy( pEnt->r.currentOrigin, out );
			for ( int i = 0; i < 3; ++i )
			{
				vec3_t tmp;
				VectorScale( axis[ i ], boxCenter[ i ], tmp );
				VectorAdd( out, tmp, out );
			}
			VectorCopy( out, _center );
			VectorCopy( axis[ 0 ], _axis0 );
			VectorCopy( axis[ 1 ], _axis1 );
			VectorCopy( axis[ 2 ], _axis2 );
			_extents[ 0 ] = ( pEnt->r.maxs[ 0 ] - pEnt->r.mins[ 0 ] ) * 0.5f;
			_extents[ 1 ] = ( pEnt->r.maxs[ 1 ] - pEnt->r.mins[ 1 ] ) * 0.5f;
			_extents[ 2 ] = ( pEnt->r.maxs[ 2 ] - pEnt->r.mins[ 2 ] ) * 0.5f;

			return Success;
		}
		return InvalidEntity;
	}

	obResult GetEntityGroundEntity( const GameEntity ent, GameEntity &moveent )
	{
		gentity_t *pEnt = EntityFromHandle( ent );
		if ( pEnt )
		{
			if ( pEnt->s.groundEntityNum > 0 && pEnt->s.groundEntityNum < ENTITYNUM_MAX_NORMAL )
			{
				moveent = HandleFromEntity( &g_entities[ pEnt->s.groundEntityNum ] );
			}
			return Success;
		}
		return InvalidEntity;
	}

	GameEntity GetEntityOwner( const GameEntity ent )
	{
		GameEntity owner;

		gentity_t *pEnt = EntityFromHandle( ent );
		if ( pEnt && pEnt->inuse )
		{
			// hack, when the game joins clients again after warmup, they are temporarily ET_GENERAL entities(LAME)
			int t = pEnt->s.eType;
			if ( pEnt->client && ( pEnt - g_entities ) < MAX_CLIENTS )
				t = ET_PLAYER;

			switch ( t )
			{
				case ET_ITEM:
				{
					if ( !Q_stricmp( pEnt->classname, "team_CTF_redflag" ) ||
						!Q_stricmp( pEnt->classname, "team_CTF_blueflag" ) )
					{
						int iFlagEntNum = pEnt - g_entities;
						for ( int i = 0; i < level.maxclients; ++i )
						{
							if ( g_entities[ i ].client && g_entities[ i ].client->flagParent == iFlagEntNum )
							{
								owner = HandleFromEntity( &g_entities[ i ] );
							}
						}
					}
					break;
				}
				case ET_GENERAL:
					break;
				/*case ET_MG42_BARREL:
				{
					if ( !Q_stricmp( pEnt->classname, "misc_mg42" ) )
					{
						if ( pEnt->r.ownerNum != pEnt->s.number )
						{
							gentity_t *pOwner = &g_entities[ pEnt->r.ownerNum ];
							if ( pOwner && pOwner->active && pOwner->client && pOwner->s.eFlags & EF_MG42_ACTIVE )
								owner = HandleFromEntity( pOwner );
						}
					}
					break;
				*/}
				default:
					// -1 means theres no owner.
					if ( pEnt->r.ownerNum < MAX_GENTITIES )
						owner = HandleFromEntity( &g_entities[ pEnt->r.ownerNum ] );
			}
		}
		return owner;
	}

	int GetEntityTeam( const GameEntity ent )
	{
		gentity_t *pEnt = EntityFromHandle( ent );
		return pEnt && pEnt->inuse ? _GetEntityTeam( pEnt ) : JA_TEAM_NONE;
	}

	obResult GetEntityForMapModel( int mapModelId, GameEntity& entityOut )
	{
		for ( int i = 0; i < m_NumDeletedMapModels; ++i )
		{
			if ( m_DeletedMapModels[ i ] == mapModelId )
			{
				return InvalidEntity;
			}
		}

		const char * mdlName = va( "*%d", mapModelId );
		const int mdlLen = strlen( mdlName );

		if ( mapModelId == 0 )
		{
			entityOut = HandleFromEntity( &g_entities[ ENTITYNUM_WORLD ] );
			return Success;
		}

		for ( int i = 0; i < level.num_entities; ++i )
		{
			if ( g_entities[ i ].inuse && g_entities[ i ].model )
			{
				if ( !Q_strncmp( mdlName, g_entities[ i ].model, mdlLen ) )
				{
					entityOut = HandleFromEntity( &g_entities[ i ] );
					return Success;
				}
			}
		}

		return InvalidEntity;
	}
	obResult GetModel( GameModelInfo & modelOut, MemoryAllocator & alloc )
	{
		fileHandle_t f;
		const int fileSize = trap->FS_Open( va( "maps/%s.bsp", level.rawmapname ), &f, FS_READ );
		if ( fileSize > 0 )
		{
			char * mapBuffer = alloc.AllocateMemory( fileSize );
			trap->FS_Read( mapBuffer, fileSize, f );
			trap->FS_Close( f );

			Com_sprintf( modelOut.mModelName, sizeof( modelOut.mModelName ), "%s.bsp", level.rawmapname );
			Com_sprintf( modelOut.mModelType, sizeof( modelOut.mModelType ), "bsp" );
			modelOut.mDataBuffer = mapBuffer;
			modelOut.mDataBufferSize = fileSize;
			return InvalidParameter;
		}
		return Success;
	}

	obResult GetWorldModel( GameModelInfo & modelOut, MemoryAllocator & alloc )
	{
		Com_sprintf( modelOut.mModelName, sizeof( modelOut.mModelName ), "maps/%s.bsp", GetMapName() );
		Com_sprintf( modelOut.mModelType, sizeof( modelOut.mModelName ), "bsp" );
		return GetModel( modelOut, alloc );
	}

	obResult GetEntityModel( const GameEntity ent, GameModelInfo & modelOut, MemoryAllocator & alloc )
	{
		gentity_t *pEnt = EntityFromHandle( ent );
		if ( pEnt )
		{
			/*if ( pEnt->s.eType == ET_GAMEMODEL )
			{
				// only this type can be scaled?
				modelOut.mScale[ 0 ] = pEnt->s.angles2[ 0 ];
				modelOut.mScale[ 1 ] = pEnt->s.angles2[ 1 ];
				modelOut.mScale[ 2 ] = pEnt->s.angles2[ 2 ];
			}*/

			if ( G_FindConfigstringByIndex( pEnt->s.modelindex, CS_MODELS, MAX_MODELS, modelOut.mModelName, sizeof( modelOut.mModelName ) ) > 0 )
			{
				const int len = strlen( modelOut.mModelName );
				for ( int i = 0; i < len; ++i )
				{
					if ( modelOut.mModelName[ i ] == '.' )
						strncpy( modelOut.mModelType, &modelOut.mModelName[ i + 1 ], sizeof( modelOut.mModelName ) );
				}

				return GetModel( modelOut, alloc );
			}
		}
		return InvalidEntity;
	}

	obResult GetCurrentWeaponClip( const GameEntity ent, FireMode _mode, int &_curclip, int &_maxclip )
	{
		gentity_t *bot = EntityFromHandle( ent );
		if ( bot && bot->inuse && bot->client )
		{
			int iWeapon = bot->client->ps.weapon;
			int iAmmoType = weaponData[iWeapon].ammoIndex;

			if (iAmmoType == AMMO_NONE || weaponData[iWeapon].energyPerShot == 0 && weaponData[iWeapon].altEnergyPerShot == 0)
			{
				_curclip = 0;
				_maxclip = 0;
				return Success;
			}

			_curclip = bot->client->ps.ammo[iAmmoType];
			_maxclip = ammoData[iAmmoType].max;

			return Success;
		}
		return InvalidEntity;
	}

	int GetCurrentWeapons( const GameEntity ent, int weaponIds [], int maxWeapons )
	{
		gentity_t *bot = EntityFromHandle( ent );
		if ( bot && bot->inuse && bot->client )
		{
			int weaponCount = 0;

			for ( int i = 0; i < WP_NUM_WEAPONS; i++ )
			{
				if ( ( bot->client->ps.stats[STAT_WEAPONS] & ( 1 << i ) ) )
				{
					weaponIds[ weaponCount ] = Bot_WeaponGameToBot( i );
					if ( weaponIds[ weaponCount ] != JA_WP_NONE )
						++weaponCount;
				}
			}
			return weaponCount;
		}
		return 0;
	}

	obResult GetCurrentAmmo( const GameEntity ent, int _weaponId, FireMode _mode, int &_cur, int &_max )
	{
		gentity_t *bot = EntityFromHandle( ent );
		if ( bot && bot->inuse && bot->client )
		{
			int maxclip = 0;
			int ammoIndex = 0;

			_weaponId = _weaponBotToGame( _weaponId );
			ammoIndex = weaponData[_weaponId].ammoIndex;

			if ( ammoIndex == AMMO_NONE || weaponData[_weaponId].energyPerShot == 0 && weaponData[_weaponId].altEnergyPerShot == 0)
			{
				_cur = 0;
				_max = 0;
				return Success;
			}

			_cur = bot->client->ps.ammo[ammoIndex];
			_max = ammoData[ammoIndex].max;

			return Success;
		}
		return InvalidEntity;
	}

	int GetGameTime()
	{
		return level.time;
	}

	void GetGoals()
	{
		gGoalSubmitReady = true;
		gentity_t *e;

		SendDeferredGoals();
		//GetMG42s();

		for ( int i = MAX_CLIENTS; i < level.num_entities; ++i )
		{
			e = &g_entities[ i ];

			if ( !e->inuse )
				continue;

			const char *pGoalName = _GetEntityName( e );

			switch ( e->s.eType )
			{
				case ET_ITEM:
				{
					if ( !Q_stricmp( e->classname, "team_CTF_redflag" ) )
					{
						// red flag
						Bot_Util_AddGoal( "flag", e, ( 1 << JA_TEAM_BLUE ), pGoalName );
					}
					else if ( !Q_stricmp( e->classname, "team_CTF_blueflag" ) )
					{
						// blue flag
						Bot_Util_AddGoal( "flag", e, ( 1 << JA_TEAM_RED ), pGoalName );
					}
					break;
				}
				case ET_OID_TRIGGER:
				{
					// detect constructibles
					char *pTmp = 0;
					gentity_t *eAxis = G_ConstructionForTeam( e, TEAM_AXIS );
					gentity_t *eAlly = G_ConstructionForTeam( e, TEAM_ALLIES );
					const char *pAllyGoalName = 0, *pAxisGoalName = 0;
					//gentity_t *mg42ent;

					//////////////////////////////////////////////////////////////////////////
					if ( eAxis )
					{
						/*if(e->targetent)
						pAxisGoalName = _GetEntityName(e->targetent);*/
						//if(!pAxisGoalName)
						//	pAxisGoalName = _GetEntityName(eAxis);
						if ( !pAxisGoalName )
							pAxisGoalName = _GetEntityName( e );
						if ( !pAxisGoalName && e->parent )
							pAxisGoalName = _GetEntityName( e->parent );
						// detect fakeobj's
						if ( strncmp( eAxis->targetname, "fakeobj", 7 ) == 0 )
							continue;
					}
					//////////////////////////////////////////////////////////////////////////
					if ( eAlly )
					{
						/*if(e->targetent)
						pAllyGoalName = _GetEntityName(e->targetent);*/
						//if(!pAllyGoalName)
						//	pAllyGoalName = _GetEntityName(eAlly);
						if ( !pAllyGoalName )
							pAllyGoalName = _GetEntityName( e );
						if ( !pAllyGoalName && e->parent )
							pAllyGoalName = _GetEntityName( e->parent );
						// detect fakeobj's
						if ( strncmp( eAlly->targetname, "fakeobj", 7 ) == 0 )
							continue;
					}

					//////////////////////////////////////////////////////////////////////////
					//if(pAxisGoalName && pAllyGoalName)
					//{
					//	if(!Q_stricmp(pAxisGoalName, pAllyGoalName))
					//	{
					//		PrintMessage("Goals Have Same Name!!");
					//		PrintMessage(pAxisGoalName);
					//	}
					//}
					//////////////////////////////////////////////////////////////////////////
					if ( eAxis && eAlly && !Q_stricmp( pAxisGoalName, pAllyGoalName ) )
					{
						//bool bMover = pAxisGoalName ? strstr(pAxisGoalName, "_construct")!=0 : false;
						bool bMover = e->target ? strstr( e->target, "_construct" ) != 0 : false;
						obUserData ud = obUserData( bMover ? 1 : 0 );
						Bot_Util_AddGoal( "build", e, ( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS ), pAxisGoalName, "Mobile", &ud );
					}
					else
					{
						if ( eAxis )
						{
							//bool bMover = pAxisGoalName ? strstr(pAxisGoalName, "_construct")!=0 : false;
							bool bMover = e->target ? strstr( e->target, "_construct" ) != 0 : false;
							obUserData ud = obUserData( bMover ? 1 : 0 );
							Bot_Util_AddGoal( "build", e, ( 1 << ET_TEAM_AXIS ), pAxisGoalName, "Mobile", &ud );
						}

						if ( eAlly )
						{
							//bool bMover = pAllyGoalName ? strstr(pAllyGoalName, "_construct")!=0 : false;
							bool bMover = e->target ? strstr( e->target, "_construct" ) != 0 : false;
							obUserData ud = obUserData( bMover ? 1 : 0 );
							Bot_Util_AddGoal( "build", e, ( 1 << ET_TEAM_ALLIES ), pAllyGoalName, "Mobile", &ud );
						}
					}
					//////////////////////////////////////////////////////////////////////////

					// register constructions as dyno targets, too
					// check if this is an enemy construction and check spawnflags if this is a dynamite target
					if ( !e->target_ent )
						continue;

					// If we didn't find a name from the entity, try to get one from its targetent
					/*pAxisGoalName = pAxisGoalName ? pAxisGoalName : _GetEntityName(e->targetent);
					pAllyGoalName = pAllyGoalName ? pAllyGoalName : _GetEntityName(e->targetent);*/
					//pAxisGoalName = _GetEntityName(e->targetent) ? _GetEntityName(e->targetent) : pAxisGoalName;
					//pAllyGoalName = _GetEntityName(e->targetent) ? _GetEntityName(e->targetent) : pAllyGoalName;
					pAxisGoalName = _GetEntityName( e ) ? _GetEntityName( e ) : pAxisGoalName;
					pAllyGoalName = _GetEntityName( e ) ? _GetEntityName( e ) : pAllyGoalName;

					//////////////////////////////////////////////////////////////////////////
					// Is this a movable?, skip it if so.
					if ( e->target_ent->targetname &&
						( pTmp = strstr( e->target_ent->targetname, "_construct" ) ) != NULL )
					{
						char strName[ 256 ];
						Q_strncpyz( strName, e->target_ent->targetname, 256 );
						strName[ pTmp - e->target_ent->targetname ] = 0;
						gentity_t *pMover = G_FindByTargetname( NULL, strName );
						if ( pMover &&
							pMover->s.eType == ET_MOVER &&
							!Q_stricmp( pMover->classname, "script_mover" ) )
							continue;
					}
					//////////////////////////////////////////////////////////////////////////

					if ( e->target_ent->s.eType == ET_CONSTRUCTIBLE )
					{
						obUserData bud;
						bud.DataType = obUserData::dtInt;
						bud.udata.mInt = 0;
						switch ( e->target_ent->constructibleStats.weaponclass )
						{
							case -1: // Grenade, fall through and also register as satchel and dyno.
							case 1: // Satchelable, it should fall through and also register as dyno.
								bud.udata.mInt |= XPLO_TYPE_SATCHEL;
							case 2: // Dynamite
								bud.udata.mInt |= XPLO_TYPE_DYNAMITE;
								break;
							default:
								// < 0 is apparently satchel only.
								if ( e->target_ent->constructibleStats.weaponclass < 0 )
									bud.udata.mInt |= XPLO_TYPE_SATCHEL;
								break;
						}

						// NOTE: sometimes goals get registered that don't have 1 or 2 weaponclass
						// is this correct? for now let's not register them and see what happens.
						if ( bud.udata.mInt != 0 )
						{
							// If there's a goal for each team, and it has the same name, just register 1 goal
							if ( eAxis && eAlly && !Q_stricmp( pAxisGoalName, pAllyGoalName ) )
							{
								Bot_Util_AddGoal( "plant", e->target_ent,
									( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS ),
									pAxisGoalName,
									"ExplosiveType",
									&bud );
								CheckForMG42( e->target_ent, pAxisGoalName );
							}
							else
							{
								if ( eAxis )
								{
									Bot_Util_AddGoal( "plant", e->target_ent,
										( 1 << ET_TEAM_ALLIES ),
										pAxisGoalName,
										"ExplosiveType",
										&bud );
									CheckForMG42( e->target_ent, pAxisGoalName );
								}

								if ( eAlly )
								{
									Bot_Util_AddGoal( "plant", e->target_ent,
										( 1 << ET_TEAM_AXIS ),
										pAllyGoalName,
										"ExplosiveType",
										&bud );
									CheckForMG42( e->target_ent, pAllyGoalName );
								}
							}
						}
						else // for grenadetarget
						{
							if ( eAxis )
							{
								Bot_Util_AddGoal( "explode",
									e->target_ent,
									( 1 << ET_TEAM_ALLIES ),
									pAxisGoalName,
									"ExplosiveType",
									&bud );
							}

							if ( eAlly )
							{
								Bot_Util_AddGoal( "explode",
									e->target_ent,
									( 1 << ET_TEAM_AXIS ),
									pAllyGoalName,
									"ExplosiveType",
									&bud );
							}
						}
					}
					// check for objective targets
					else if ( e->target_ent->s.eType == ET_EXPLOSIVE )
					{
						obUserData bud;
						switch ( e->target_ent->constructibleStats.weaponclass )
						{
							case 1:
							{
								// SATCHEL
								bud.udata.mInt |= XPLO_TYPE_SATCHEL;
								// note ALLIED/AXIS stuff is reversed here (ET is strange sometimes)
								/*if (e->spawnflags & ALLIED_OBJECTIVE)
								Bot_Util_AddGoal("satcheltarget",
								e->targetent,
								(1<<ET_TEAM_AXIS),
								pAxisGoalName);
								if (e->spawnflags & AXIS_OBJECTIVE)
								Bot_Util_AddGoal("satcheltarget",
								e->targetent,
								(1<<ET_TEAM_ALLIES),
								pAllyGoalName);*/
							}
							// fall through to register dyna too.
							case 2:
							{
								// DYNAMITE
								bud.udata.mInt |= XPLO_TYPE_DYNAMITE;
								// note ALLIED/AXIS stuff is reversed here (ET is strange sometimes)
								if ( e->spawnflags & ALLIED_OBJECTIVE )
									Bot_Util_AddGoal( "plant",
									e->target_ent,
									( 1 << ET_TEAM_AXIS ),
									pAxisGoalName,
									"ExplosiveType",
									&bud );
								if ( e->spawnflags & AXIS_OBJECTIVE )
									Bot_Util_AddGoal( "plant",
									e->target_ent,
									( 1 << ET_TEAM_ALLIES ),
									pAllyGoalName,
									"ExplosiveType",
									&bud );
								break;
							}
						}
					}
					break;
				}
				case ET_GENERAL:
					break;
				/*case ET_MOVER:
				{
					int team = 0;
					if ( e->spawnflags & 32 )
						team = ( 1 << ET_TEAM_ALLIES );
					else if ( e->spawnflags & 64 )
						team = ( 1 << ET_TEAM_AXIS );
					else
					{
						team |= ( 1 << ET_TEAM_ALLIES );
						team |= ( 1 << ET_TEAM_AXIS );
					}
					pGoalName = _GetEntityName( e );
					if ( pGoalName && !Q_stricmp( e->classname, "script_mover" ) &&
						e->think != G_FreeEntity )
					{
						Bot_Util_AddGoal( "mover", e, team, pGoalName );
					}
					break;
				}*/
				//case ET_CABINET_H:
				//	{
				//		int team = 0;
				//		team |= (1 << ET_TEAM_ALLIES);
				//		team |= (1 << ET_TEAM_AXIS);
				//		pGoalName = _GetEntityName(e);
				//		//Bot_Util_AddGoal(e, GOAL_HEALTH, team, pGoalName, NULL);
				//		break;
				//	}
				//case ET_CABINET_A:
				//	{
				//		int team = 0;
				//		team |= (1 << ET_TEAM_ALLIES);
				//		team |= (1 << ET_TEAM_AXIS);
				//		pGoalName = _GetEntityName(e);
				//		//Bot_Util_AddGoal(e, GOAL_AMMO, team, pGoalName, NULL);
				//		break;
				//	}
#if 0
				case ET_HEALER:
				{
					obUserData bud( e->damage ); // heal rate
					int team = ( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS );
					pGoalName = _GetEntityName( e->target_ent ) ? _GetEntityName( e->target_ent ) : _GetEntityName( e );
					Bot_Util_AddGoal( "healthcab", e, team, pGoalName, "HealRate", &bud );
					break;
				}
				case ET_SUPPLIER:
				{
					obUserData bud( e->damage ); // heal rate
					int team = ( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS );
					pGoalName = _GetEntityName( e->target_ent ) ? _GetEntityName( e->target_ent ) : _GetEntityName( e );
					Bot_Util_AddGoal( "ammocab", e, team, pGoalName, "AmmoRate", &bud );
					break;
				}
				case ET_TRAP:
				{
					if ( !Q_stricmp( e->classname, "team_WOLF_checkpoint" ) )
					{
						Bot_Util_AddGoal( "checkpoint", e, 0, pGoalName );
					}
					break;
				}
				case ET_EXPLOSIVE:
				{
					//CS: hack - weaponclass not set yet, using xpbonus
					if ( !Q_stricmp( e->classname, "func_explosive" ) && e->constructibleStats.constructxpbonus == 5 )
					{
						pGoalName = _GetEntityName( e );
						Bot_Util_AddGoal( "explosive", e, 0, pGoalName );
					}
					break;
				}
#endif
				default:
				{
					break;
				}
			}
		}

		/*for ( int i = 0; i < numofmg42s; ++i )
		{
			char strName[ 256 ];

			if ( mg42s[ i ].buildable )
				strcpy( strName, mg42s[ i ].newname );
			else
				strcpy( strName, mg42s[ i ].name );

			Bot_Util_AddGoal( "mountmg42", mg42s[ i ].ent,
				( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS ), strName );

			// cs: this was done in et_goalmanager before
			Bot_Util_AddGoal( "repairmg42", mg42s[ i ].ent,
				( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS ), strName );
		}*/
	}

	void GetPlayerInfo( obPlayerInfo &info )
	{
		info.mAvailableTeams |= ( 1 << JA_TEAM_RED );
		info.mAvailableTeams |= ( 1 << JA_TEAM_BLUE );

		info.mMaxPlayers = level.maxclients;

		GameEntity ge;

		for ( int i = 0; i < level.maxclients; ++i )
		{
			if ( !g_entities[ i ].inuse )
				continue;
			if ( !g_entities[ i ].client )
				continue;
			if ( g_entities[ i ].client->pers.connected != CON_CONNECTED )
				continue;

			ge = HandleFromEntity( &g_entities[ i ] );
			if ( GetOmnibotEntityType( &g_entities[ i ], info.mPlayers[ i ].mEntInfo ) )
			{
				info.mPlayers[ i ].mTeam = GetEntityTeam( ge );
				info.mPlayers[ i ].mController = IsBot( &g_entities[ i ] ) ? obPlayerInfo::Bot : obPlayerInfo::Human;
			}
		}
	}

	void PrintError( const char *_error )
	{
		if ( _error )
			trap->Print( "%s%s\n", S_COLOR_RED, _error );
	}

	void PrintMessage( const char *_msg )
	{
		if ( _msg )
		{
			// et console doesn't support tabs, so
			const int BufferSize = 1024;
			char tmpbuffer[ BufferSize ] = {};
			const char *src = _msg;
			char *dest = tmpbuffer;
			while ( *src != 0 )
			{
				if ( *src == '\t' )
				{
					for ( int i = 0; i < 4; ++i )
						*dest++ = ' ';
					src++;
				}
				else
				{
					*dest++ = *src++;
				}
			}
			trap->Print( "%s%s\n", S_COLOR_GREEN, tmpbuffer );
		}
	}

	//bool PrintScreenText(const float _pos[3], float _duration, const obColor &_color, const char *_msg)
	//{
	//	if(_msg && (g_LastScreenMessageTime != level.time))
	//	{
	//		trap_SendServerCommand(-1, va("cp \"%s\"", _msg));
	//		g_LastScreenMessageTime = level.time;
	//	}

	//	//gentity_t *pEnt = &level.g_entities[0];

	//	//// for dedicated servers we quit early because there can't possibly be a client 0 that is local client
	//	//if(g_dedicated.integer)
	//	//	return;

	//	//if(pEnt && pEnt->inuse && pEnt->client && pEnt->client->pers.localClient && !IsBot(pEnt) && _msg)
	//	//{
	//	//	if(_pos)
	//	//	{
	//	//		trap_SendServerCommand(0, va("wp \"%s\" %f %f %f %f", _msg, _pos[0], _pos[1], _pos[2], _duration));
	//	//	}
	//	//	else
	//	//	{
	//	//		if(g_LastScreenMessageTime != level.time)
	//	//		{
	//	//			trap_SendServerCommand(0, va("cp \"%s\"", _msg));
	//	//			g_LastScreenMessageTime = level.time;
	//	//		}
	//	//	}
	//	//}
	//}

	const char *GetMapName()
	{
		return level.rawmapname;
	}

	void GetMapExtents( AABB &_aabb )
	{
#if 0
		if ( level.mapcoordsValid )
		{
			_aabb.mMins[ 0 ] = level.mapcoordsMins[ 0 ] /** 2.F*/;
			_aabb.mMins[ 1 ] = level.mapcoordsMins[ 1 ] /** 2.F*/;
			_aabb.mMins[ 2 ] = -65535.0f;
			_aabb.mMaxs[ 0 ] = level.mapcoordsMaxs[ 0 ] /** 2.F*/;
			_aabb.mMaxs[ 1 ] = level.mapcoordsMaxs[ 1 ] /** 2.F*/;
			_aabb.mMaxs[ 2 ] = 65535.0f;

			for ( int i = 0; i < 3; ++i )
			{
				if ( _aabb.mMins[ i ] > _aabb.mMaxs[ i ] )
				{
					float t = _aabb.mMins[ i ];
					_aabb.mMins[ i ] = _aabb.mMaxs[ i ];
					_aabb.mMaxs[ i ] = t;
				}
			}
		}
		else
#endif
		{
			memset( &_aabb, 0, sizeof( AABB ) );
		}
	}

	GameEntity EntityByName( const char *_name )
	{
		gentity_t *pEnt = G_FindByTargetname( NULL, _name );
		return HandleFromEntity( pEnt );
	}

	GameEntity EntityFromID( const int _gameId )
	{
		gentity_t *pEnt = INDEXENT( _gameId );
		return pEnt ? HandleFromEntity( pEnt ) : GameEntity();
	}

	int IDFromEntity( const GameEntity ent )
	{
		gentity_t *pEnt = EntityFromHandle( ent );
		if ( pEnt )
		{
			gentity_t *pStart = g_entities;
			int iIndex = pEnt - pStart;
			assert( iIndex >= 0 );
			return ( iIndex < MAX_GENTITIES ) ? iIndex : -1;
		}
		return -1;
	}

	bool DoesEntityStillExist( const GameEntity &_hndl )
	{
		return _hndl.IsValid() ? EntityFromHandle( _hndl ) != NULL : false;
	}

	int GetAutoNavFeatures( AutoNavFeature *_feature, int _max )
	{
		int iNumFeatures = 0;
		for ( int i = MAX_CLIENTS; i < level.num_entities && iNumFeatures < _max; ++i )
		{
			gentity_t *e = &g_entities[ i ];

			if ( !e->inuse )
				continue;

			_feature[ iNumFeatures ].mEntityInfo = EntityInfo();

			////////////////////////////////////////////////////////////////////////
			_feature[ iNumFeatures ].mTravelTime = 0;
			_feature[ iNumFeatures ].mObstacleEntity = false;
			for ( int i = 0; i < 3; ++i )
			{
				_feature[ iNumFeatures ].mPosition[ i ] = e->r.currentOrigin[ i ];
				_feature[ iNumFeatures ].mTargetPosition[ i ] = e->r.currentOrigin[ i ];
				_feature[ iNumFeatures ].mBounds.mMins[ 0 ] = 0.f;
				_feature[ iNumFeatures ].mBounds.mMaxs[ 0 ] = 0.f;
				AngleVectors( e->s.angles, _feature[ iNumFeatures ].mFacing, NULL, NULL );
			}

			_feature[ iNumFeatures ].mBounds.mMins[ 0 ] = e->r.absmin[ 0 ];
			_feature[ iNumFeatures ].mBounds.mMins[ 1 ] = e->r.absmin[ 1 ];
			_feature[ iNumFeatures ].mBounds.mMins[ 2 ] = e->r.absmin[ 2 ];
			_feature[ iNumFeatures ].mBounds.mMaxs[ 0 ] = e->r.absmax[ 0 ];
			_feature[ iNumFeatures ].mBounds.mMaxs[ 1 ] = e->r.absmax[ 1 ];
			_feature[ iNumFeatures ].mBounds.mMaxs[ 2 ] = e->r.absmax[ 2 ];
			//////////////////////////////////////////////////////////////////////////
			if ( e->classname )
			{
				if ( !Q_stricmp( e->classname, "team_CTF_redspawn" ) )
					_feature[ iNumFeatures ].mEntityInfo.mGroup = ENT_GRP_PLAYERSTART;
				else if ( !Q_stricmp( e->classname, "team_CTF_bluespawn" ) )
					_feature[ iNumFeatures ].mEntityInfo.mGroup = ENT_GRP_PLAYERSTART;
				else if ( !Q_stricmp( e->classname, "info_player_deathmatch" ) ||
					!Q_stricmp( e->classname, "info_player_spawn" ) )
				{
					_feature[ iNumFeatures ].mEntityInfo.mGroup = ENT_GRP_PLAYERSTART;
				}
				else if ( !Q_stricmp( e->classname, "target_teleporter" ) )
				{
					_feature[ iNumFeatures ].mEntityInfo.mGroup = ENT_GRP_TELEPORTER;

					gentity_t *pTarget = G_PickTarget( e->target );
					if ( pTarget )
					{
						_feature[ iNumFeatures ].mTargetPosition[ 0 ] = pTarget->r.currentOrigin[ 0 ];
						_feature[ iNumFeatures ].mTargetPosition[ 1 ] = pTarget->r.currentOrigin[ 1 ];
						_feature[ iNumFeatures ].mTargetPosition[ 2 ] = pTarget->r.currentOrigin[ 2 ];
					}
				}
				else if ( !Q_stricmp( e->classname, "team_CTF_redflag" ) )
				{
					_feature[ iNumFeatures ].mEntityInfo.mGroup = ENT_GRP_GOAL;
				}
				else if ( !Q_stricmp( e->classname, "team_CTF_blueflag" ) )
				{
					_feature[ iNumFeatures ].mEntityInfo.mGroup = ENT_GRP_GOAL;
				}
				/*else if ( !Q_stricmp( e->classname, "misc_mg42" ) || !Q_stricmp( e->classname, "misc_mg42base" ) )
				{
				_feature[ iNumFeatures ].mType = ET_CLASSEX_MG42MOUNT;
				_feature[ iNumFeatures ].mObstacleEntity = true;
				}
				else if ( !Q_stricmp( e->classname, "misc_cabinet_health" ) )
				{
				_feature[ iNumFeatures ].mType = ET_CLASSEX_HEALTHCABINET;
				_feature[ iNumFeatures ].mObstacleEntity = true;
				}
				else if ( !Q_stricmp( e->classname, "misc_cabinet_supply" ) )
				{
				_feature[ iNumFeatures ].mType = ET_CLASSEX_AMMOCABINET;
				_feature[ iNumFeatures ].mObstacleEntity = true;
				}*/
			}

			if ( _feature[ iNumFeatures ].mEntityInfo.mGroup != 0 )
			{
				iNumFeatures++;
			}
		}
		return iNumFeatures;
	}

	const char *GetGameName()
	{
		return GAME_VERSION;
	}

	const char *GetModName()
	{
		return OMNIBOT_MODNAME;
	}

	const char *GetModVers()
	{
		return OMNIBOT_MODVERSION;
	}

	const char *GetBotPath()
	{
		return Omnibot_GetLibraryPath();
	}

	const char *GetLogPath()
	{
		static char logpath[ 512 ];
		trap_Cvar_VariableStringBuffer( "fs_homepath", logpath, sizeof( logpath ) );
		return Omnibot_FixPath( logpath );
	}

	bool AreCheatsEnabled()
	{
		return g_cheats.integer != 0;
	}

	bool GetGravity( float& outGravity )
	{
		outGravity = g_gravity.value;
		return true;
	}

	bool IsAlive( const GameEntity ent )
	{
		gentity_t * e0 = EntityFromHandle( ent );
		if ( e0 && e0->inuse && e0->client && e0->health > 0 && e0->client->ps.pm_type == PM_NORMAL )
		{
			return true;
		}
		return false;
	}

	bool IsAllied( const GameEntity ent1, const GameEntity ent2 )
	{
		gentity_t * e1 = EntityFromHandle( ent1 );
		gentity_t * e2 = EntityFromHandle( ent2 );
		if ( e1 && e2 )
		{
			if ( ENTINDEX( e2 ) == ENTITYNUM_WORLD )
				return true;
			else
			{
				return _GetEntityTeam( e1 ) == _GetEntityTeam( e2 );
			}
		}
		return false;
	}

	bool IsOutSide( const float pos[ 3 ] )
	{
		trace_t tr;
		vec3_t end = { pos[ 0 ], pos[ 1 ], ( pos[ 2 ] + 4096 ) };
		trap_Trace( &tr, pos, NULL, NULL, end, -1, MASK_SOLID );
		if ( ( tr.fraction < 1.0 ) && !( tr.surfaceFlags & SURF_NOIMPACT ) )
			return false;
		else
			return true;
	}

	bool IsReadyToFire( const GameEntity ent )
	{
		gentity_t * e0 = EntityFromHandle( ent );
		if ( e0 && e0->inuse && e0->client )
		{
			if ( e0->client->ps.weaponstate == WEAPON_READY || e0->client->ps.weaponstate == WEAPON_FIRING )
			{
				return true;
			}
		}
		return true;
	}

	bool IsReloading( const GameEntity ent )
	{
		gentity_t * e0 = EntityFromHandle( ent );
		if ( ( e0 && e0->inuse && e0->client && e0->client->ps.weaponstate >= WEAPON_RAISING ) &&
			( e0->client->ps.weaponstate <= WEAPON_RELOADING ) )
		{
			return true;
		}
		return false;
	}

	bool IsWeaponCharged( const GameEntity ent, int weaponId, FireMode _mode )
	{
		gentity_t * e0 = EntityFromHandle( ent );
		if ( e0 && e0->inuse && e0->client )
		{
#ifdef NOQUARTER
			if ( weaponId == ET_WP_BINOCULARS && ( pEnt->client->ps.ammo[ WP_ARTY ] & NO_ARTY ) )
			{
				return false;
			}
			else if ( weaponId == ET_WP_SMOKE_MARKER && ( pEnt->client->ps.ammo[ WP_ARTY ] & NO_AIRSTRIKE ) )
			{
				return false;
			}
			else
			{
#endif
				return weaponCharged( &e0->client->ps, e0->client->sess.sessionTeam, _weaponBotToGame( weaponId ), e0->client->sess.skill );
#ifdef NOQUARTER
			}
#endif
		}
		return true;
	}

	bool GetMaxSpeed( const GameEntity ent, float & outSpeed )
	{
		outSpeed = (float)g_movespeed.integer;
		return true;
	}

	bool GetEquippedWeapon( const GameEntity ent, int& outWeaponId, FireMode& outFireMode )
	{
		gentity_t * e0 = EntityFromHandle( ent );
		if ( e0 && e0->inuse && e0->client )
		{
			outWeaponId = Bot_WeaponGameToBot( e0->client->ps.weapon );
			outFireMode = Primary;
			return true;
		}
		return false;
	}

	bool GetMountedWeapon( const GameEntity ent, int& outWeaponId, FireMode& outFireMode )
	{
		gentity_t * e0 = EntityFromHandle( ent );
		if ( e0 && e0->inuse && e0->client )
		{
			outWeaponId = BG_PlayerMounted( e0->s.eFlags ) ? ET_WP_MOUNTABLE_MG42 : ET_WP_NONE;
			outFireMode = Primary;
			return true;
		}
		return false;
	}

	bool GetWeaponLimits( const GameEntity ent, int weaponId, ParamsWeaponLimits& outLimits )
	{
		gentity_t * e0 = EntityFromHandle( ent );
		if ( e0 && e0->inuse && e0->client )
		{
			outLimits.mLimited = false;

			if ( weaponId == ET_WP_MOUNTABLE_MG42 && BG_PlayerMounted( e0->client->ps.eFlags ) )
			{
				outLimits.mLimited = true;

				AngleVectors( e0->client->pmext.centerangles, outLimits.mCenterFacing, NULL, NULL );
				if ( e0->client->ps.eFlags & EF_MOUNTEDTANK )
				{
					// seems tanks have complete horizonal movement, and fixed vertical
					outLimits.mMinYaw = -360;
					outLimits.mMaxYaw = -360;
					outLimits.mMinPitch = 14;
					outLimits.mMaxPitch = 50;
				}
				else
				{
					outLimits.mMinYaw = -e0->client->pmext.harc;
					outLimits.mMaxYaw = e0->client->pmext.harc;
					outLimits.mMinPitch = -e0->client->pmext.varc;
					outLimits.mMaxPitch = e0->client->pmext.varc;
				}
			}

			if ( weaponId == ET_WP_MOBILE_MG42 || weaponId == ET_WP_MOBILE_MG42_SET )
			{
				AngleVectors( e0->client->pmext.mountedWeaponAngles, outLimits.mCenterFacing, NULL, NULL );
				outLimits.mLimited = true;
				outLimits.mMinYaw = -20.f;
				outLimits.mMaxYaw = 20.f;
				outLimits.mMinPitch = -20.f;
				outLimits.mMaxPitch = 20.f;
			}
			return true;
		}
		return false;
	}

	bool GetWeaponHeat( const GameEntity ent, int weaponId, FireMode firemode, float& curheat, float& maxheat )
	{
		gentity_t * e0 = EntityFromHandle( ent );
		if ( e0 && e0->inuse && e0->client )
		{
			if ( weaponId == ET_WP_MOUNTABLE_MG42 )
			{
				curheat = e0->client->ps.weapHeat[ WP_DUMMY_MG42 ];
				maxheat = MAX_MG42_HEAT;
			}
			else
			{
				curheat = e0->client->ps.weapHeat[ _weaponBotToGame( weaponId ) ];
#ifdef NOQUARTER
				maxheat = GetWeaponTableData( _weaponBotToGame( weaponId ) )->maxHeat;
#else
				maxheat = GetAmmoTableData( _weaponBotToGame( weaponId ) )->maxHeat;
#endif
			}
		}
		return false;
	}

	bool GetFlagState( const GameEntity ent, FlagState& outState, GameEntity& outCarrier )
	{
		// todo:
		return false;
	}

	int GetControllingTeam( const GameEntity ent )
	{
		gentity_t * e0 = EntityFromHandle( ent );
		if ( e0 && e0->s.eType == ET_TRAP )
			return Bot_TeamGameToBot( e0->count );
		return ET_TEAM_NONE;
	}

	int GetGameType() const
	{
		return g_gametype.integer;
	}

	int GetGameTimeLeft() const
	{
		const int iTimeLimit = (int)( g_timelimit.value * 60000 );
		const int iMatchTime = level.timeCurrent - level.startTime;
		return ( iTimeLimit - iMatchTime );
	}
	GameState GetGameState() const
	{
		if ( level.framenum < GAME_INIT_FRAMES )
			return GAME_STATE_INVALID;

		switch ( g_gamestate.integer )
		{
			case GS_PLAYING:
				return GAME_STATE_PLAYING;
			case GS_WARMUP_COUNTDOWN:
				return GAME_STATE_WARMUP_COUNTDOWN;
			case GS_WARMUP:
				return GAME_STATE_WARMUP;
			case GS_INTERMISSION:
				return GAME_STATE_INTERMISSION;
			case GS_WAITING_FOR_PLAYERS:
				return GAME_STATE_WAITINGFORPLAYERS;
		}
		return GAME_STATE_INVALID;
	}

	bool ServerCommand( const char* cmd )
	{
		trap_SendConsoleCommand( EXEC_NOW, cmd );
		return true;
	}

	bool ServerSetVariable( const char* name, const char* value )
	{
		trap_Cvar_Set( name, value );
		return true;
	}

	bool ServerGetVariable( const char* name, char* value, size_t valueMaxLen )
	{
		trap_Cvar_VariableStringBuffer( name, value, (int)valueMaxLen );
		return true;
	}

	bool ServerScriptEvent( const char* func, const char* entName, const obUserData params [], size_t numParams )
	{
		return false;
	}

	bool ChangeName( const GameEntity ent, const char* newName )
	{
		// todo:
		return false;
	}

	bool Suicide( const GameEntity ent )
	{
		// todo:
		return false;
	}

	bool PlaySound( const GameEntity ent, const char* name )
	{
		/*CHL2MP_Player * player = ToHL2MPPlayer( EntityFromHandle( ent ) );
		if ( player )
		{
			player->EmitSound( name );
			return true;
		}*/
		return false;
	}

	bool StopSound( const GameEntity ent, const char* name )
	{
		/*CHL2MP_Player * player = ToHL2MPPlayer( EntityFromHandle( ent ) );
		if ( player )
		{
			player->StopSound( name );
			return true;
		}*/
		return false;
	}

	bool GetVehicleInfo( const GameEntity ent, ParamsVehicleInfo& outVehicleInfo )
	{
		return false;
	}

	bool IsMoverAt( const float pos1[ 3 ], const float pos2[ 3 ], GameEntity& outMover )
	{
		/*Vector org( pos1[ 0 ], pos1[ 1 ], pos1[ 2 ] );
		Vector under( pos2[ 0 ], pos2[ 1 ], pos2[ 2 ] );

		trace_t tr;
		unsigned int iMask = MASK_PLAYERSOLID_BRUSHONLY;
		UTIL_TraceLine( org, under, iMask, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

		if ( tr.DidHitNonWorldEntity() &&
			!tr.m_pEnt->IsPlayer() &&
			!tr.startsolid )
		{
			outMover = omnibot_interface::HandleFromEntity( tr.m_pEnt );
			return true;
		}*/
		return false;
	}

	// Enemy Territory Specifics
	ExplosiveState GetExplosiveState( const GameEntity ent )
	{
		gentity_t * xplo = EntityFromHandle( ent );
		if ( xplo && xplo->inuse )
		{
			if ( xplo->s.eType == ET_MISSILE )
			{
				switch ( xplo->s.weapon )
				{
					case WP_DYNAMITE:
					case WP_LANDMINE:
						return ( xplo->s.teamNum < 4 ) ? XPLO_ARMED : XPLO_UNARMED;
					case WP_SATCHEL:
						return ( xplo->health >= 250 ) ? XPLO_UNARMED : XPLO_ARMED;
				}
			}
		}
		return XPLO_INVALID;
	}

	ConstructableState GetConstructableState( const GameEntity ent, const GameEntity constructable )
	{
		gentity_t * player = EntityFromHandle( ent );
		gentity_t * constr = EntityFromHandle( constructable );
		if ( player && player->inuse && constr && constr->inuse )
		{
			if ( G_IsConstructible( player->client->sess.sessionTeam, constr ) )
				return CONST_UNBUILT;
			else if ( ( constr = G_ConstructionForTeam( constr, player->client->sess.sessionTeam ) ) && G_ConstructionIsFullyBuilt( constr ) )
				return CONST_BUILT;
			else
				return CONST_INVALID;
		}
		return CONST_INVALID;
	}
	ConstructableState IsDestroyable( const GameEntity ent, const GameEntity destroyable )
	{
		gentity_t * player = EntityFromHandle( ent );
		gentity_t * dest = EntityFromHandle( destroyable );
		if ( player && player->inuse && dest && dest->inuse )
		{
			if ( dest->s.eType == ET_OID_TRIGGER )
				dest = dest->target_ent;

			// uhh, whoops. oid without a target ent. happens on map urbanterritory.
			if ( !dest )
			{
				return CONST_NOTDESTROYABLE;
			}

			if ( dest->s.eType == ET_CONSTRUCTIBLE )
			{
				//qboolean d = G_ConstructionIsDestroyable(pDestroyable);

				if ( dest->spawnflags & CONSTRUCTIBLE_INVULNERABLE )
					return CONST_INVALID;
				else if ( G_ConstructionIsPartlyBuilt( dest ) && dest->s.teamNum != player->client->sess.sessionTeam )
					return CONST_DESTROYABLE;
				else if ( dest->parent && dest->parent->s.eType == ET_OID_TRIGGER )
				{
					gentity_t *pCurrent = dest->parent->chain;
					while ( pCurrent )
					{
						if ( G_ConstructionIsPartlyBuilt( pCurrent ) &&
							pCurrent->s.teamNum != player->client->sess.sessionTeam )
						{
							return CONST_DESTROYABLE;
						}
						pCurrent = pCurrent->chain;
						if ( pCurrent == dest->parent->chain )
							break;
					}
				}
				else
					return CONST_NOTDESTROYABLE;
			}
			else if ( dest->s.eType == ET_EXPLOSIVE && dest->parent && dest->parent->s.eType == ET_OID_TRIGGER &&
				( ( ( dest->parent->spawnflags & ALLIED_OBJECTIVE ) && player->client->sess.sessionTeam == TEAM_AXIS ) ||
				( ( dest->parent->spawnflags & AXIS_OBJECTIVE ) && player->client->sess.sessionTeam == TEAM_ALLIES ) ) )
			{
				if ( dest->health > 0 )
					return CONST_DESTROYABLE;
				else
					return CONST_NOTDESTROYABLE;
			}
		}
		return CONST_INVALID;
	}
	bool IsCarryingFlag( const GameEntity ent )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->inuse && player->client && ( player->health >= 0 ) )
		{
			if ( player->client->ps.powerups[ PW_REDFLAG ] || player->client->ps.powerups[ PW_BLUEFLAG ] )
			{
				return true;
			}
		}
		return false;
	}
	bool ItemCanBeGrabbed( const GameEntity ent, const GameEntity item )
	{
		gentity_t * player = EntityFromHandle( ent );
		gentity_t * grab = EntityFromHandle( item );
		if ( player && player->client && grab && grab->inuse )
		{
			// DUPLICATE ERROR CHECK, so BG_CanItemBeGrabbed doesn't screw up.
			if ( grab->s.modelindex < 1 || grab->s.modelindex >= bg_numItems )
			{
				//Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );
				return false;
			}
			else
			{
				qboolean canGrab = BG_CanItemBeGrabbed( &grab->s, &player->client->ps,
					player->client->sess.skill, player->client->sess.sessionTeam );

				// When flags aren't dropped, we need to reject some pickup cases
				// This is because the return flag goal uses this function to see if they can
				// grab/return a flag
				if ( canGrab && !( grab->flags & FL_DROPPED_ITEM ) )
				{
					if ( ( player->client->sess.sessionTeam == TEAM_AXIS ) &&
						!Q_stricmp( grab->classname, "team_CTF_redflag" ) )
					{
						GameEntity owner = GetEntityOwner( item );
						if ( !owner.IsValid() )
							canGrab = qfalse;
					}
					else if ( ( player->client->sess.sessionTeam == TEAM_ALLIES ) &&
						!Q_stricmp( grab->classname, "team_CTF_blueflag" ) )
					{
						GameEntity owner = GetEntityOwner( item );
						if ( !owner.IsValid() )
							canGrab = qfalse;
					}
				}

				return canGrab != qfalse;
			}
		}
		return false;
	}
	bool TeamMineCount( const GameEntity ent, int & current, int & max )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->client )
		{
			current = G_CountTeamLandmines( player->client->sess.sessionTeam );
#ifdef NOQUARTER
			max = team_maxLandmines.integer;
#elif defined ETPUB_VERSION
			max = g_maxTeamLandmines.integer;
#else
			max = MAX_TEAM_LANDMINES;
#endif
			return true;
		}
		return false;
	}
	bool IsWaitingForMedic( const GameEntity ent )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->inuse && player->client &&
			( player->health <= 60 || player->client->ps.pm_type == PM_DEAD ) &&
			!( player->client->ps.pm_flags & PMF_LIMBO ) )
		{
			return true;
		}
		else if ( player && player->inuse && player->client && player->client->ps.pm_type == PM_NORMAL )
		{
			return false;
		}
		return false;
	}
	bool SelectWeapon( LoadOut load, const GameEntity ent, ET_Weapon weapon )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->inuse && player->client )
		{
			if ( weapon == ET_WP_GPG40 )
				weapon = ET_WP_KAR98;
			else if ( weapon == ET_WP_M7 )
				weapon = ET_WP_CARBINE;

			if ( load == LoadOutSecondary )
			{
				player->client->sess.playerWeapon2 = _weaponBotToGame( weapon );
				player->client->sess.latchPlayerWeapon2 = _weaponBotToGame( weapon );
			}
			else
			{
				//CS: /kill 2 seconds before next spawn
				if ( !( player->client->ps.pm_flags & PMF_LIMBO ) && player->client->sess.playerWeapon != _weaponBotToGame( weapon ) )
				{
					player->client->sess.botSuicide = qtrue;
				}

#ifdef NOQUARTER
				// dupe weapons now have same id for NQ
				if ( player->client->sess.sessionTeam == TEAM_ALLIES && weapon == ET_WP_MOBILE_MG42 )
				{
					player->client->sess.playerWeapon = WP_MOBILE_BROWNING;
					player->client->sess.latchPlayerWeapon = WP_MOBILE_BROWNING;
				}
				else if ( player->client->sess.sessionTeam == TEAM_AXIS && weapon == ET_WP_MORTAR )
				{
					player->client->sess.playerWeapon = WP_MORTAR2;
					player->client->sess.latchPlayerWeapon = WP_MORTAR2;
				}
				else if ( player->client->sess.sessionTeam == TEAM_ALLIES && weapon == ET_WP_PANZERFAUST )
				{
					player->client->sess.playerWeapon = WP_BAZOOKA;
					player->client->sess.latchPlayerWeapon = WP_BAZOOKA;
				}
				else
				{
					player->client->sess.playerWeapon = _weaponBotToGame( weapon );
					player->client->sess.latchPlayerWeapon = _weaponBotToGame( weapon );
				}
#else // !NOQUARTER
				player->client->sess.playerWeapon = _weaponBotToGame( weapon );
				player->client->sess.latchPlayerWeapon = _weaponBotToGame( weapon );
#endif
			}
			return true;
		}
		return false;
	}
	int GetReinforceTime( const GameEntity ent )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->inuse && player->client )
		{
			if ( player->client->sess.sessionTeam == TEAM_AXIS )
			{
				return g_redlimbotime.integer -
					( ( level.dwRedReinfOffset + level.timeCurrent - level.startTime ) %
					g_redlimbotime.integer );
			}
			else if ( player->client->sess.sessionTeam == TEAM_ALLIES )
			{
				return g_bluelimbotime.integer -
					( ( level.dwBlueReinfOffset + level.timeCurrent - level.startTime ) %
					g_bluelimbotime.integer );
			}
		}
		return -1;
	}
	bool IsMedicNear( const GameEntity ent )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->inuse && player->client &&
			player->client->ps.pm_type == PM_DEAD && !( player->client->ps.pm_flags & PMF_LIMBO ) )
		{
			return ( player->client->ps.viewlocked == 7 );
		}
		return false;
	}
	bool GoToLimbo( const GameEntity ent )
	{
		gentity_t * player = EntityFromHandle( ent );
		int limbo_health = GIB_HEALTH;
		if ( player && player->inuse && player->client && player->health > limbo_health &&
			player->client->ps.pm_type == PM_DEAD && !( player->client->ps.pm_flags & PMF_LIMBO ) )
		{
			limbo( player, qtrue );
			return true;
		}
		return false;
	}
	GameEntity GetMountedPlayerOnMG42( const GameEntity gun )
	{
		gentity_t * gunEntity = EntityFromHandle( gun );
		if ( gunEntity && gunEntity->inuse && ( gunEntity->r.ownerNum < level.maxclients ) )
			return HandleFromEntity( &g_entities[ gunEntity->r.ownerNum ] );
		return GameEntity();
	}
	bool IsMountableGunRepairable( const GameEntity ent, const GameEntity gun )
	{
		gentity_t * player = EntityFromHandle( ent );
		gentity_t * gunEntity = EntityFromHandle( gun );
		if ( player && player->inuse && gunEntity && gunEntity->inuse )
			return G_EmplacedGunIsRepairable( gunEntity, player ) == qtrue;
		return false;
	}
	int GetGunHealth( const GameEntity gun )
	{
		gentity_t * gunEntity = EntityFromHandle( gun );
		if ( gunEntity && gunEntity->inuse && gunEntity->r.linked && gunEntity->entstate == STATE_DEFAULT )
		{
			if ( gunEntity->mg42BaseEnt != -1 )
				return g_entities[ gunEntity->mg42BaseEnt ].health;
			else
				return gunEntity->health;
		}
		return -1;
	}
	bool GetCurrentCursorHint( const GameEntity ent, int & type, int & val )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->inuse )
		{
			type = CURSOR_HINT_NONE;
			val = player->client->ps.serverCursorHintVal;

			switch ( player->client->ps.serverCursorHint )
			{
				case HINT_PLAYER:
					type = CURSOR_HINT_PLAYER;
					break;
				case HINT_ACTIVATE:
					type = CURSOR_HINT_ACTIVATE;
					break;
				case HINT_DOOR:
					type = CURSOR_HINT_DOOR;
					break;
				case HINT_DOOR_ROTATING:
					type = CURSOR_HINT_DOOR_ROTATING;
					break;
				case HINT_DOOR_LOCKED:
					type = CURSOR_HINT_DOOR_LOCKED;
					break;
				case HINT_DOOR_ROTATING_LOCKED:
					type = CURSOR_HINT_DOOR_LOCKED;
					break;
				case HINT_MG42:
					type = CURSOR_HINT_MG42;
					break;
				case HINT_BREAKABLE:
					type = CURSOR_HINT_BREAKABLE;
					break;
				case HINT_BREAKABLE_DYNAMITE:
					type = CURSOR_HINT_BREAKABLE_DYNAMITE;
					break;
				case HINT_CHAIR:
					type = CURSOR_HINT_CHAIR;
					break;
				case HINT_ALARM:
					type = CURSOR_HINT_ALARM;
					break;
				case HINT_HEALTH:
					type = CURSOR_HINT_HEALTH;
					break;
#ifndef NOQUARTER
				case HINT_TREASURE:
					type = CURSOR_HINT_TREASURE;
					break;
#endif
				case HINT_KNIFE:
					type = CURSOR_HINT_KNIFE;
					break;
				case HINT_LADDER:
					type = CURSOR_HINT_LADDER;
					break;
				case HINT_BUTTON:
					type = CURSOR_HINT_BUTTON;
					break;
				case HINT_WATER:
					type = CURSOR_HINT_WATER;
					break;
#ifndef NOQUARTER
				case HINT_CAUTION:
					type = CURSOR_HINT_CAUTION;
					break;
				case HINT_DANGER:
					type = CURSOR_HINT_DANGER;
					break;
				case HINT_SECRET:
					type = CURSOR_HINT_SECRET;
					break;
				case HINT_QUESTION:
					type = CURSOR_HINT_QUESTION;
					break;
				case HINT_EXCLAMATION:
					type = CURSOR_HINT_EXCLAMATION;
					break;
				case HINT_CLIPBOARD:
					type = CURSOR_HINT_CLIPBOARD;
					break;
#endif
				case HINT_WEAPON:
					type = CURSOR_HINT_WEAPON;
					break;
				case HINT_AMMO:
					type = CURSOR_HINT_AMMO;
					break;
#ifndef NOQUARTER
				case HINT_ARMOR:
					type = CURSOR_HINT_ARMOR;
					break;
#endif
				case HINT_POWERUP:
					type = CURSOR_HINT_POWERUP;
					break;
#ifndef NOQUARTER
				case HINT_HOLDABLE:
					type = CURSOR_HINT_HOLDABLE;
					break;
#endif
				case HINT_INVENTORY:
					type = CURSOR_HINT_INVENTORY;
					break;
				case HINT_SCENARIC:
					type = CURSOR_HINT_SCENARIC;
					break;
				case HINT_EXIT:
					type = CURSOR_HINT_EXIT;
					break;
				case HINT_NOEXIT:
					type = CURSOR_HINT_NOEXIT;
					break;
				case HINT_PLYR_FRIEND:
					type = CURSOR_HINT_PLYR_FRIEND;
					break;
				case HINT_PLYR_NEUTRAL:
					type = CURSOR_HINT_PLYR_NEUTRAL;
					break;
				case HINT_PLYR_ENEMY:
					type = CURSOR_HINT_PLYR_ENEMY;
					break;
				case HINT_PLYR_UNKNOWN:
					type = CURSOR_HINT_PLYR_UNKNOWN;
					break;
				case HINT_BUILD:
					type = CURSOR_HINT_BUILD;
					break;
				case HINT_DISARM:
					type = CURSOR_HINT_DISARM;
					break;
				case HINT_REVIVE:
					type = CURSOR_HINT_REVIVE;
					break;
				case HINT_DYNAMITE:
					type = CURSOR_HINT_DYNAMITE;
					break;
				case HINT_CONSTRUCTIBLE:
					type = CURSOR_HINT_CONSTRUCTIBLE;
					break;
				case HINT_UNIFORM:
					type = CURSOR_HINT_UNIFORM;
					break;
				case HINT_LANDMINE:
					type = CURSOR_HINT_LANDMINE;
					break;
				case HINT_TANK:
					type = CURSOR_HINT_TANK;
					break;
				case HINT_SATCHELCHARGE:
					type = CURSOR_HINT_SATCHELCHARGE;
					break;
#ifndef NOQUARTER
				case HINT_LOCKPICK:
					type = CURSOR_HINT_LOCKPICK;
					break;
#endif
			}
			return true;
		}
		return false;
	}
	bool ChangeSpawnPoint( const GameEntity ent, int spawnpoint )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->client )
		{
			SetPlayerSpawn( player, spawnpoint, qtrue );
			return true;
		}
		return false;
	}
	bool GetMg42Properties( const GameEntity ent, ParamsMG42Info &data )
	{
		gentity_t * gun = EntityFromHandle( ent );
		if ( gun && gun->inuse && gun->client && BG_PlayerMounted( gun->client->ps.eFlags ) )
		{
			AngleVectors( gun->client->pmext.centerangles, data.mCenterFacing, NULL, NULL );
			//AngleVectors(pEnt->client->pmext.mountedWeaponAngles, pMsg->, NULL, NULL);
			if ( gun->client->ps.eFlags & EF_MOUNTEDTANK )
			{
				// seems tanks have complete horizonal movement, and fixed vertical
				data.mMinHorizontalArc = -360;
				data.mMaxHorizontalArc = -360;
				data.mMinVerticalArc = 14;
				data.mMaxVerticalArc = 50;
			}
			else
			{
				data.mMinHorizontalArc = -gun->client->pmext.harc;
				data.mMaxHorizontalArc = gun->client->pmext.harc;
				data.mMinVerticalArc = -gun->client->pmext.varc;
				data.mMaxVerticalArc = gun->client->pmext.varc;
			}
			return true;
		}
		return false;
	}
	bool GetCabinetData( GameEntity ent, ParamsCabinetData& data )
	{
		gentity_t * cabinet = EntityFromHandle( ent );
		if ( cabinet && ( cabinet->s.eType == ET_HEALER || cabinet->s.eType == ET_SUPPLIER ) )
		{
			data.mRate = cabinet->damage;
			data.mCurrentAmount = cabinet->health;
			data.mMaxAmount = cabinet->count;
			return true;
		}
		return false;
	}
	void DisableBotPush( const GameEntity ent, bool push )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->client )
		{
			player->client->sess.botPush = push ? qtrue : qfalse;
		}
	}
	bool GetSkills( const GameEntity ent, ParamsSkills& skills )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->client )
		{
			skills.mSkill[ ET_SKILL_BATTLE_SENSE ] = player->client->sess.skill[ SK_BATTLE_SENSE ];
			skills.mSkill[ ET_SKILL_ENGINEERING ] = player->client->sess.skill[ SK_EXPLOSIVES_AND_CONSTRUCTION ];
			skills.mSkill[ ET_SKILL_FIRST_AID ] = player->client->sess.skill[ SK_FIRST_AID ];
			skills.mSkill[ ET_SKILL_SIGNALS ] = player->client->sess.skill[ SK_SIGNALS ];
			skills.mSkill[ ET_SKILL_LIGHT_WEAPONS ] = player->client->sess.skill[ SK_LIGHT_WEAPONS ];
			skills.mSkill[ ET_SKILL_HEAVY_WEAPONS ] = player->client->sess.skill[ SK_HEAVY_WEAPONS ];
			skills.mSkill[ ET_SKILL_COVERTOPS ] = player->client->sess.skill[ SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS ];
			return true;
		}
		return false;
	}

	virtual bool FireTeamCreate( const GameEntity ent )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->client )
			trap_EA_Command( player - g_entities, ( char* ) "fireteam create" );
		return true;
	}
	virtual bool FireTeamDisband( const GameEntity ent )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->client )
			trap_EA_Command( player - g_entities, ( char* ) "fireteam disband" );
		return true;
	}
	virtual bool FireTeamLeave( const GameEntity ent )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->client )
			trap_EA_Command( player - g_entities, ( char* ) "fireteam leave" );
		return true;
	}
	virtual bool FireTeamApply( const GameEntity ent, int fireteamnum )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->client )
			trap_EA_Command( player - g_entities, va( "fireteam apply %i", fireteamnum ) );
		return true;
	}
	virtual bool FireTeamInvite( const GameEntity ent, GameEntity target )
	{
		gentity_t * player = EntityFromHandle( ent );
		gentity_t * targ = EntityFromHandle( target );
		if ( player && player->client && targ && targ->inuse )
			trap_EA_Command( player - g_entities, va( "fireteam invite %i", ( targ - g_entities ) + 1 ) );
		return true;
	}
	virtual bool FireTeamWarn( const GameEntity ent, GameEntity target )
	{
		gentity_t * player = EntityFromHandle( ent );
		gentity_t * targ = EntityFromHandle( target );
		if ( player && player->client && targ && targ->inuse )
			trap_EA_Command( player - g_entities, va( "fireteam warn %i", ( targ - g_entities ) + 1 ) );
		return true;
	}
	virtual bool FireTeamKick( const GameEntity ent, GameEntity target )
	{
		gentity_t * player = EntityFromHandle( ent );
		gentity_t * targ = EntityFromHandle( target );
		if ( player && player->client && targ && targ->inuse )
			trap_EA_Command( player - g_entities, va( "fireteam kick %i", ( targ - g_entities ) + 1 ) );
		return true;
	}
	virtual bool FireTeamPropose( const GameEntity ent, GameEntity target )
	{
		gentity_t * player = EntityFromHandle( ent );
		gentity_t * targ = EntityFromHandle( target );
		if ( player && player->client && targ && targ->inuse )
			trap_EA_Command( player - g_entities, va( "fireteam propose %i", ( targ - g_entities ) + 1 ) );
		return true;
	}
	virtual int FireTeamGetInfo( const GameEntity ent, int& fireTeamNum, GameEntity& leader, GameEntity members[ 64 ] )
	{
		gentity_t * player = EntityFromHandle( ent );
		if ( player && player->client )
		{
			fireteamData_t* ft = 0;
			if ( G_IsOnFireteam( player - g_entities, &ft ) )
			{
				fireTeamNum = ft->ident;
				leader = HandleFromEntity( &g_entities[ (int)ft->joinOrder[ 0 ] ] );
				
				int mbrnum = 0;
				for ( int i = 0; i < g_maxclients.integer; ++i )
				{
					members[ mbrnum ].Reset();
					if ( ft->joinOrder[ i ] != -1 )
					{
						GameEntity ge = HandleFromEntity( &g_entities[ (int)ft->joinOrder[ i ] ] );
						if ( ge.IsValid() )
							members[ mbrnum++ ] = ge;
					}
				}

				return mbrnum;
			}
		}
		return true;
	}
	bool GetLocationName( const float pos[ 3 ], char buffer[], size_t bufferSize )
	{
		return false;
	}
};

void Bot_Interface_InitHandles()
{
	for ( int i = 0; i < MAX_GENTITIES; ++i )
	{
		mEntityHandles[ i ].mHandleSerial = 1;
		mEntityHandles[ i ].mNewEntity = false;
		mEntityHandles[ i ].mUsed = false;
	}
}

int Bot_Interface_Init()
{
	if ( omnibot_enable.integer == 0 )
	{
		trap->Print( "%s%s\n", S_COLOR_GREEN,
			"Omni-bot is currently disabled with \"omnibot_enable 0\"" );
		return 1;
	}

//#ifdef _DEBUG
//	trap->Cvar_Set( "sv_cheats", "1" );
//	trap->Cvar_Update( &g_cheats );
//#endif

	gGoalSubmitReady = false;

	gGameFunctions = new JAInterface;
	omnibot_error err = Omnibot_LoadLibrary( JA_VERSION_LATEST,
		"omnibot_ja", Omnibot_FixPath( omnibot_path.string ) );
	if ( err == BOT_ERROR_NONE )
	{
		return true;
	}
	return false;
}

int Bot_Interface_Shutdown()
{
	if ( IsOmnibotLoaded() )
	{
		gBotFunctions->Shutdown();
	}
	Omnibot_FreeLibrary();
	return 1;
}

//////////////////////////////////////////////////////////////////////////

void Bot_Interface_ConsoleCommand()
{
	enum
	{
		BuffSize = 32
	};
	char buffer[ BuffSize ] = {};
	trap->Argv( 1, buffer, BuffSize );

	if ( IsOmnibotLoaded() )
	{
		if ( !Q_stricmp( buffer, "unload" ) )
		{
			Bot_Interface_Shutdown();
			return;
		}
		else if ( !Q_stricmp( buffer, "reload" ) )
		{
			Bot_Interface_Shutdown();
			Bot_Interface_InitHandles();
			Bot_Interface_Init();
			return;
		}

		Arguments args;
		for ( int i = 0; i < trap->Argc(); ++i )
		{
			trap->Argv( i, args.mArgs[ args.mNumArgs++ ], Arguments::MaxArgLength );
		}
		gBotFunctions->ConsoleCommand( args );
	}
	else
	{
		if ( !Q_stricmp( buffer, "load" ) )
		{
			Bot_Interface_InitHandles();
			Bot_Interface_Init();
			return;
		}
		else
		{
			trap->Print( "%s%s\n", S_COLOR_RED, "Omni-bot not loaded." );
		}
	}
}

//extern "C" void script_mover_spawn( gentity_t *ent );

void Bot_Interface_Update()
{
	if ( IsOmnibotLoaded() )
	{
		//#ifdef _DEBUG
		//		trap_Cvar_Set( "sv_cheats", "1" );
		//		trap_Cvar_Update(&g_cheats);
		//#endif
		/*if (level.framenum == GAME_INIT_FRAMES)
		Bot_Event_StartGame();*/

		//////////////////////////////////////////////////////////////////////////
		{
			// time triggers for Omni-bot
#ifdef NOQUARTER
			if ( level.framenum % sv_fps.integer == 0 )
#else
			if ( level.framenum % 20 == 0 ) //@sv_fps
#endif
			{
				if ( !level.twoMinute && ( g_timelimit.value * 60000 - ( level.time - level.startTime ) ) < 120000 )
				{
					level.twoMinute = qtrue;
					Bot_Util_SendTrigger( NULL, NULL, "two minute warning.", "twominute" );
				}

				if ( !level.thirtySecond && ( g_timelimit.value * 60000 - ( level.time - level.startTime ) ) < 30000 )
				{
					level.thirtySecond = qtrue;
					Bot_Util_SendTrigger( NULL, NULL, "thirty second warning.", "thirtysecond" );
				}
			}
		}
		//////////////////////////////////////////////////////////////////////////
		int iNumBots = 0;
		for ( int i = 0; i < level.maxclients; ++i )
		{
			if ( !g_entities[ i ].inuse )
				continue;
			if ( !g_entities[ i ].client )
				continue;
			if ( g_entities[ i ].client->pers.connected != CON_CONNECTED )
				continue;

			/*if(i==1)
			g_entities[i].flags |= FL_GODMODE;*/

			// Send a spectated message to bots that are being spectated.
			if ( ( g_entities[ i ].client->sess.sessionTeam == TEAM_SPECTATOR ) &&
				( g_entities[ i ].client->sess.spectatorState == SPECTATOR_FOLLOW ) )
			{
				const int iDestination = g_entities[ i ].client->sess.spectatorClient;
				Bot_Event_Spectated( &g_entities[ iDestination ], i );
			}

			// fake handle server commands (to prevent server command overflow)
			if ( ( g_entities[ i ].inuse == qtrue ) && IsBot( &g_entities[ i ] ) )
			{
				++iNumBots;

				char buf[ 1024 ] = { 0 };
				while ( trap->BotGetServerCommand( i, buf, sizeof( buf ) ) )
				{
				}
			}
		}

		if ( !( omnibot_flags.integer & OBF_DONT_SHOW_BOTCOUNT ) )
		{
			if ( omnibot_playing.integer != iNumBots )
			{
				trap->Cvar_Set( "omnibot_playing", va( "%i", iNumBots ) );
				trap->Cvar_Update( &omnibot_playing );
			}
		}
		else
		{
			if ( omnibot_playing.integer != -1 )
			{
				trap->Cvar_Set( "omnibot_playing", "-1" );
				trap->Cvar_Update( &omnibot_playing );
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Register any pending entity updates.
		for ( int i = 0; i < MAX_GENTITIES; ++i )
		{
			if ( mEntityHandles[ i ].mNewEntity && g_entities[ i ].inuse )
			{
				//if ( g_entities[ i ].think != script_mover_spawn )
				{
					mEntityHandles[ i ].mNewEntity = false;
					Bot_EntityCreated( &g_entities[ i ] );
				}
			}
		}
		SendDeferredGoals();
		//////////////////////////////////////////////////////////////////////////
		// Call the libraries update.
		gBotFunctions->Update();
		//////////////////////////////////////////////////////////////////////////
	}
}

//////////////////////////////////////////////////////////////////////////

qboolean Bot_Util_AllowPush( int weaponId )
{
	switch ( weaponId )
	{
		case WP_MORTAR_SET:
#ifdef NOQUARTER
		case WP_MORTAR2_SET:
		case WP_MOBILE_BROWNING_SET:
		case WP_BAR_SET:
#endif
		case WP_MOBILE_MG42_SET:
			return qfalse;
	}
	return qtrue;
}

//////////////////////////////////////////////////////////////////////////
qboolean Bot_Util_CheckForSuicide( gentity_t *ent )
{
	if ( ent && ent->client )
	{
		// Omni-bot: used for class changes, bot will /kill 2 seconds before spawn
		if ( ent->client->sess.botSuicide == qtrue )
		{
			if ( ent->client->sess.sessionTeam == TEAM_AXIS )
			{
				if ( ( g_redlimbotime.integer - ( ( level.dwRedReinfOffset + level.timeCurrent - level.startTime ) % g_redlimbotime.integer ) ) < 2000 )
				{
					Cmd_Kill_f( ent );
					ent->client->sess.botSuicide = qfalse;
					return qtrue;
				}
			}
			else if ( ent->client->sess.sessionTeam == TEAM_ALLIES )
			{
				if ( ( g_bluelimbotime.integer - ( ( level.dwBlueReinfOffset + level.timeCurrent - level.startTime ) % g_bluelimbotime.integer ) ) < 2000 )
				{
					Cmd_Kill_f( ent );
					ent->client->sess.botSuicide = qfalse;
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

//////////////////////////////////////////////////////////////////////////

void Bot_Event_ClientConnected( gentity_t * _player, qboolean _isbot )
{
	if ( IsOmnibotLoaded() )
	{
		EvClientConnected::Msg event;
		event.mData.mGameId = ENTINDEX( _player );
		event.mData.mIsBot = _isbot == qtrue;
		gBotFunctions->SendEvent( event, GameEntity() );

		Analytic_Event( event.mData.mIsBot ? "Connected:Bot" : "Connected:Human", NULL );
	}
}

void Bot_Event_ClientDisConnected( gentity_t * _player )
{
	if ( IsOmnibotLoaded() )
	{
		EvClientDisconnected::Msg event;
		event.mData.mGameId = ENTINDEX( _player );
		gBotFunctions->SendEvent( event, GameEntity() );

		Analytic_Event( IsBot( _player ) ? "Disconnected:Bot" : "Disconnected:Human", NULL );
	}
}

void Bot_Event_Spawn( gentity_t * _player )
{
	if ( IsOmnibotLoaded() )
	{
		if ( IsBot( _player ) )
		{
			EvSpawn::Msg event;
			gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );
		}

		Analytic_Event( IsBot( _player ) ? "Spawn:Bot" : "Spawn:Human", _player ? _player->r.currentOrigin : NULL );
	}
}

void Bot_Event_TakeDamage( gentity_t * _player, gentity_t * _attacker, const char * damagetype, float damage )
{
	if ( IsOmnibotLoaded() )
	{
		EvTakeDamage::Msg event;
		event.mData.mInflictor = HandleFromEntity( _attacker );
		Q_strncpyz( event.mData.mDamageType, damagetype ? damagetype : "<unknown>", sizeof( event.mData.mDamageType ) );
		if ( IsBot( _player ) )
			gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );

		Analytic_EventWithValue( va( "Damage:%s:%s", IsBot( _player ) ? "Bot" : "Human", event.mData.mDamageType ),
			damage, _player ? _player->r.currentOrigin : NULL );
	}
}

void Bot_Event_Death( gentity_t * _victim, gentity_t * _killer, const char * damagetype )
{
	if ( IsOmnibotLoaded() )
	{
		EvDeath::Msg event;
		event.mData.mWhoKilledMe = HandleFromEntity( _killer );
		Q_strncpyz( event.mData.mMeansOfDeath, damagetype ? damagetype : "<unknown>", sizeof( event.mData.mMeansOfDeath ) );
		if ( IsBot( _victim ) )
			gBotFunctions->SendEvent( event, HandleFromEntity( _victim ) );

		Analytic_Event( va( "Death:%s:%s", IsBot( _victim ) ? "Bot" : "Human", event.mData.mMeansOfDeath ),
			_victim ? _victim->r.currentOrigin : NULL );
	}
}

void Bot_Event_Healed( gentity_t * _player, gentity_t * _whodoneit )
{
	if ( IsOmnibotLoaded() )
	{
		if ( IsBot( _player ) )
		{
			EvHealed::Msg event;
			event.mData.mWhoHealedMe = HandleFromEntity( _whodoneit );
			gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );
		}

		Analytic_Event( va( "Healed:%s", IsBot( _player ) ? "Bot" : "Human" ), _player ? _player->r.currentOrigin : NULL );
	}
}

void Bot_Event_RecievedAmmo( gentity_t * _player, gentity_t * _whodoneit )
{
	if ( IsOmnibotLoaded() )
	{
		if ( IsBot( _player ) )
		{
			EvRecievedAmmo::Msg event;
			event.mData.mFromWho = HandleFromEntity( _whodoneit );
			gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );
		}

		Analytic_Event( va( "RecievedAmmo:%s", IsBot( _player ) ? "Bot" : "Human" ), _player ? _player->r.currentOrigin : NULL );
	}
}

void Bot_Event_Revived( gentity_t * _player, gentity_t * _whodoneit )
{
	if ( IsOmnibotLoaded() )
	{
		if ( IsBot( _player ) )
		{
			EvRevived::Msg event;
			event.mData.mWhoRevivedMe = HandleFromEntity( _whodoneit );
			gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );
		}

		Analytic_Event( va( "Revived:%s", IsBot( _player ) ? "Bot" : "Human" ), _player ? _player->r.currentOrigin : NULL );
	}
}

void Bot_Event_KilledSomeone( gentity_t * _attacker, gentity_t * _victim, const char *_meansofdeath )
{
	if ( IsOmnibotLoaded() )
	{
		if ( _attacker )
		{
			EvKilledSomeone::Msg event;
			Q_strncpyz( event.mData.mMeansOfDeath,
				_meansofdeath ? _meansofdeath : "<unknown>", sizeof( event.mData.mMeansOfDeath ) / sizeof( event.mData.mMeansOfDeath[ 0 ] ) );

			event.mData.mWhoIKilled = HandleFromEntity( _victim );
			gBotFunctions->SendEvent( event, HandleFromEntity( _attacker ) );

			Analytic_Event( va( "Kill:%s:%s", IsBot( _attacker ) ? "Bot" : "Human", event.mData.mMeansOfDeath ),
				_attacker ? _attacker->r.currentOrigin : NULL );
		}
	}
}

void Bot_Event_FireWeapon( gentity_t * _player, int _weaponId, gentity_t * _projectile )
{
	if ( IsOmnibotLoaded() )
	{
		EvWeaponFire::Msg event;
		event.mData.mWeaponId = _weaponId;
		event.mData.mFireMode = Primary;
		event.mData.mProjectile = HandleFromEntity( _projectile );
		gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );

		Analytic_Event( va( "WeaponFired(%d)", _weaponId ), _player ? _player->r.currentOrigin : NULL );
	};
}

void Bot_Event_PreTriggerMine( gentity_t * _player, gentity_t *_mine )
{
	if ( IsOmnibotLoaded() )
	{
		if ( IsBot( _player ) )
		{
			EvMineArmed::Msg event;
			event.mData.mMine = HandleFromEntity( _mine );
			gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );
		}

		Analytic_Event( va( "MineArmed:%s", IsBot( _player ) ? "Bot" : "Human" ), _player ? _player->r.currentOrigin : NULL );
	}
}

void Bot_Event_PostTriggerMine( gentity_t * _player, gentity_t *_mine )
{
	if ( IsOmnibotLoaded() )
	{
		if ( IsBot( _player ) )
		{
			EvMineDetonate::Msg event;
			event.mData.mMine = HandleFromEntity( _mine );
			gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );
		}

		Analytic_Event( va( "MineDetonate:%s", IsBot( _player ) ? "Bot" : "Human" ), _player ? _player->r.currentOrigin : NULL );
	}
}

void Bot_Event_MortarImpact( gentity_t * _player, vec3_t _pos, float radius )
{
	if ( IsOmnibotLoaded() )
	{
		if ( IsBot( _player ) )
		{
			EvMortarImpact::Msg event;
			event.mData.mOwner = HandleFromEntity( _player );
			event.mData.mPosition[ 0 ] = _pos[ 0 ];
			event.mData.mPosition[ 1 ] = _pos[ 1 ];
			event.mData.mPosition[ 2 ] = _pos[ 2 ];
			event.mData.mRadius = radius;

			gBotFunctions->SendEvent( event, GameEntity() );
		}

		Analytic_Event( va( "MortarDetonate:%s", IsBot( _player ) ? "Bot" : "Human" ), _pos );
	}
}

void Bot_Event_Spectated( gentity_t * _player, int _who )
{
	if ( IsOmnibotLoaded() )
	{
		if ( IsBot( _player ) )
		{
			EvSpectated::Msg event;
			event.mData.mWhoSpectatingMe = _who;
			gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );
		}
	}
}

void Bot_Event_ChatMessage( gentity_t * _player, gentity_t *_source, int _type, const char *_message )
{
	if ( IsOmnibotLoaded() )
	{
		if ( IsBot( _player ) )
		{
			switch ( _type )
			{
				case SAY_ALL:
				{
					EvChatMessageGlobal::Msg event;
					event.mData.mWhoSaidIt = HandleFromEntity( _source );
					Q_strncpyz( event.mData.mMessage, _message ? _message : "<unknown>",
						sizeof( event.mData.mMessage ) / sizeof( event.mData.mMessage[ 0 ] ) );
					gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );
					break;
				}
				case SAY_TEAM:
				case SAY_TEAMNL:
				{
					EvChatMessageTeam::Msg event;
					event.mData.mWhoSaidIt = HandleFromEntity( _source );
					Q_strncpyz( event.mData.mMessage, _message ? _message : "<unknown>",
						sizeof( event.mData.mMessage ) / sizeof( event.mData.mMessage[ 0 ] ) );
					gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );
					break;
				}
				case SAY_BUDDY:
				{
					EvChatMessagePrivate::Msg event;
					event.mData.mWhoSaidIt = HandleFromEntity( _source );
					Q_strncpyz( event.mData.mMessage, _message ? _message : "<unknown>",
						sizeof( event.mData.mMessage ) / sizeof( event.mData.mMessage[ 0 ] ) );
					gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );
					break;
				}
			}
		}
	}
}

void Bot_Event_VoiceMacro( gentity_t * _player, gentity_t *_source, int _type, const char *_message )
{
	if ( IsOmnibotLoaded() )
	{
		if ( IsBot( _player ) )
		{
			switch ( _type )
			{
				case SAY_ALL:
				{
					EvVoiceMacroGlobal::Msg event;
					event.mData.mWhoSaidIt = HandleFromEntity( _source );
					Q_strncpyz( event.mData.mMessage, _message ? _message : "<unknown>",
						sizeof( event.mData.mMessage ) / sizeof( event.mData.mMessage[ 0 ] ) );
					gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );
					break;
				}
				case SAY_TEAM:
				case SAY_TEAMNL:
				{
					EvVoiceMacroTeam::Msg event;
					event.mData.mWhoSaidIt = HandleFromEntity( _source );
					Q_strncpyz( event.mData.mMessage, _message ? _message : "<unknown>",
						sizeof( event.mData.mMessage ) / sizeof( event.mData.mMessage[ 0 ] ) );
					gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );
					break;
				}
				case SAY_BUDDY:
				{
					EvVoiceMacroPrivate::Msg event;
					event.mData.mWhoSaidIt = HandleFromEntity( _source );
					Q_strncpyz( event.mData.mMessage, _message ? _message : "<unknown>",
						sizeof( event.mData.mMessage ) / sizeof( event.mData.mMessage[ 0 ] ) );
					gBotFunctions->SendEvent( event, HandleFromEntity( _player ) );
					break;
				}
			}
		}

		Analytic_Event( va( "VoiceMacro:%s", IsBot( _source ) ? "Bot" : "Human" ), _source ? _source->r.currentOrigin : NULL );
	}
}

void Bot_Event_Sound( gentity_t *_source, int _sndtype, const char *_name )
{
	if ( IsOmnibotLoaded() )
	{
		EvSound::Msg event;
		event.mData.mSource = HandleFromEntity( _source );
		event.mData.mSoundType = _sndtype;
		gGameFunctions->GetEntityPosition( event.mData.mSource, event.mData.mOrigin );
		Q_strncpyz( event.mData.mSoundName, _name ? _name : "<unknown>", sizeof( event.mData.mSoundName ) / sizeof( event.mData.mSoundName[ 0 ] ) );
		gBotFunctions->SendEvent( event, GameEntity() );
	}
}

void Analytic_Event( const char * eventName, const float * optionalPos )
{
	EvAnalyticEvent::Msg event;
	event.mData.mHasValue = false;
	event.mData.mArea = gGameFunctions->GetMapName();
	if ( event.mData.mArea != NULL && event.mData.mArea[ 0 ] != NULL )
	{
		event.mData.mName = eventName;
		event.mData.mHasValue = false;
		if ( optionalPos != NULL )
		{
			event.mData.mPosition[ 0 ] = optionalPos[ 0 ];
			event.mData.mPosition[ 1 ] = optionalPos[ 1 ];
			event.mData.mPosition[ 2 ] = optionalPos[ 2 ];
			event.mData.mHasPosition = true;
		}
		else
		{
			event.mData.mHasPosition = false;
		}
		gBotFunctions->SendEvent( event, GameEntity() );
	}
}

void Analytic_EventWithValue( const char * eventName, float value, const float * optionalPos )
{
	EvAnalyticEvent::Msg event;
	event.mData.mHasValue = true;
	event.mData.mValue = value;
	event.mData.mArea = gGameFunctions->GetMapName();
	if ( event.mData.mArea != NULL && event.mData.mArea[ 0 ] != NULL )
	{
		event.mData.mName = eventName;
		event.mData.mHasValue = false;
		if ( optionalPos != NULL )
		{
			event.mData.mPosition[ 0 ] = optionalPos[ 0 ];
			event.mData.mPosition[ 1 ] = optionalPos[ 1 ];
			event.mData.mPosition[ 2 ] = optionalPos[ 2 ];
			event.mData.mHasPosition = true;
		}
		else
		{
			event.mData.mHasPosition = false;
		}
		gBotFunctions->SendEvent( event, GameEntity() );
	}
}

void Bot_EntityCreated( gentity_t * ent )
{
	GameEntity gameEnt = HandleFromEntity( ent );
	Bot_Util_CheckForGoalEntity( gameEnt );

	//////////////////////////////////////////////////////////////////////////
	// Cache smoke bombs
	/*if ( ( ent->s.eType == ET_MISSILE && ent->s.weapon == WP_SMOKE_BOMB ) )
	{
		for ( int i = 0; i < MAX_SMOKEGREN_CACHE; ++i )
		{
			if ( !g_SmokeGrenadeCache[ i ] )
			{
				g_SmokeGrenadeCache[ i ] = ent;
				break;
			}
		}
	}*/
}

void Bot_EntityQueued( gentity_t *pEnt )
{
	if ( pEnt && IsOmnibotLoaded() )
	{
		const int entIndex = pEnt - g_entities;
		mEntityHandles[ entIndex ].mNewEntity = true;
		mEntityHandles[ entIndex ].mUsed = true;
	}
}

void Bot_EntityDeleted( gentity_t *pEnt )
{
	if ( pEnt )
	{
		if ( pEnt->model && pEnt->model[ 0 ] == '*' && m_NumDeletedMapModels < MAX_MODELS )
		{
			m_DeletedMapModels[ m_NumDeletedMapModels++ ] = atoi( &pEnt->model[ 1 ] );
		}

		const int iEntNum = ENTINDEX( pEnt );
		if ( IsOmnibotLoaded() )
		{
			gBotFunctions->EntityDestroyed( GameEntity( iEntNum, mEntityHandles[ iEntNum ].mHandleSerial ) );
		}

		mEntityHandles[ iEntNum ].mUsed = false;
		mEntityHandles[ iEntNum ].mNewEntity = false;
		while ( ++mEntityHandles[ iEntNum ].mHandleSerial == 0 )
		{
		}
	}
	/*for ( int i = 0; i < MAX_SMOKEGREN_CACHE; ++i )
	{
		if ( g_SmokeGrenadeCache[ i ] == pEnt )
		{
			g_SmokeGrenadeCache[ i ] = NULL;
		}
	}*/
}

//////////////////////////////////////////////////////////////////////////

void Bot_Util_SendTrigger( gentity_t *ent, gentity_t *_activator, const char *_tagname, const char *_action )
{
	if ( IsOmnibotLoaded() )
	{
		TriggerInfo triggerInfo;
		triggerInfo.mEntity = HandleFromEntity( ent );
		Q_strncpyz( triggerInfo.mTagName, _tagname, TriggerBufferSize );
		Q_strncpyz( triggerInfo.mAction, _action, TriggerBufferSize );
		gBotFunctions->SendTrigger( triggerInfo );
	}
}

//////////////////////////////////////////////////////////////////////////

bool GetOmnibotEntityType( gentity_t * ent, EntityInfo & classInfo )
{
	classInfo = EntityInfo();

	classInfo.mGroup = ENT_GRP_UNKNOWN;
	
	if ( ( ent - g_entities ) < MAX_CLIENTS )
	{
		classInfo.mGroup = ENT_GRP_PLAYER;

		classInfo.mCategory.SetFlag( ENT_CAT_SHOOTABLE, true );
		classInfo.mCategory.SetFlag( ENT_CAT_PLAYER, true );

		classInfo.mFlags.SetFlag( ENT_FLAG_VISTEST );

		switch ( ent->client->sess.latchPlayerType )
		{
			case PC_SOLDIER:
				classInfo.mClassId = ET_CLASS_SOLDIER;
				break;
			case PC_MEDIC:
				classInfo.mClassId = ET_CLASS_MEDIC;
				break;
			case PC_ENGINEER:
				classInfo.mClassId = ET_CLASS_ENGINEER;
				break;
			case PC_FIELDOPS:
				classInfo.mClassId = ET_CLASS_FIELDOPS;
				break;
			case PC_COVERTOPS:
				classInfo.mClassId = ET_CLASS_COVERTOPS;
				break;
			default:
				classInfo.mClassId = ET_CLASS_NULL;
				classInfo.mCategory.ClearAll();
				break;
		}

		classInfo.mHealth.Set( ent->client->ps.stats[ STAT_HEALTH ], ent->client->ps.stats[ STAT_MAX_HEALTH ] );
	}
	else
	{
		// is it the world entity
		if ( ent == &g_entities[ ENTITYNUM_WORLD ] )
		{
			classInfo.mGroup = ENT_GRP_MAP;
			classInfo.mCategory.SetFlag( ENT_CAT_OBSTACLE );
			classInfo.mFlags.SetFlag( ENT_FLAG_COLLIDABLE );
			return true;
		}

		classInfo.mHealth.Set( ent->takedamage ? ent->health : 0, 0 );

		switch ( ent->s.eType )
		{
			case ET_MOVER:
			{
				classInfo.mGroup = ENT_GRP_MOVER;
				classInfo.mCategory.SetFlag( ENT_CAT_OBSTACLE );
				classInfo.mFlags.SetFlag( ENT_FLAG_COLLIDABLE );

				classInfo.mHealth.Set( ent->takedamage ? ent->health > 0 ? ent->health : 0 : 0, ent->count );

				if ( !Q_stricmp( ent->classname, "script_mover" ) )
				{
					if ( ent->count > 0 )
						classInfo.mClassId = ( ent->spawnflags & 4 ) ? ET_CLASSEX_VEHICLE_HVY : ET_CLASSEX_VEHICLE;
					else //if(_ent->model2)
						classInfo.mClassId = ET_CLASSEX_VEHICLE_NODAMAGE;
				}
				/*else if (!Q_stricmp(pCurrent->classname, "props_flamebarrel"))
				{
				info.mEntityClass = ET_CLASSEX_BREAKABLE;
				info.mEntityCategory = ENT_CAT_SHOOTABLE;
				}
				else if (!Q_stricmp(pCurrent->classname, "props_statue"))
				{
				info.mEntityClass = ET_CLASSEX_BREAKABLE;
				info.mEntityCategory = ENT_CAT_SHOOTABLE;
				}*/
				else if ( !Q_stricmp( ent->classname, "props_chair_hiback" ) ||
					!Q_stricmp( ent->classname, "props_chair" ) ||
					!Q_stricmp( ent->classname, "props_chair_side" ) )
				{
					if ( ( ent->health > 0 ) && ( ent->takedamage == qtrue ) )
					{
						classInfo.mClassId = ET_CLASSEX_BREAKABLE;
					}
				}
				break;
			}
			case ET_CONSTRUCTIBLE:
				classInfo.mGroup = ENT_GRP_PROP;
				classInfo.mClassId = ET_CLASSEX_CONSTRUCTIBLE;
				classInfo.mHealth.Set( ent->takedamage ? ent->health > 0 ? ent->health : 0 : 0, ent->count );
				classInfo.mCategory.SetFlag( ENT_CAT_OBSTACLE );

				if ( G_ConstructionIsFullyBuilt( ent ) )
					classInfo.mFlags.SetFlag( ENT_FLAG_COLLIDABLE );

				//if (G_ConstructionIsPartlyBuilt(pCurrent) &&
				//	!(pCurrent->spawnflags & CONSTRUCTIBLE_INVULNERABLE) &&
				//	 (pCurrent->constructibleStats.weaponclass == 0))
				//{
				//	info.mEntityClass = ET_CLASSEX_BREAKABLE;
				//	info.mEntityCategory = ENT_CAT_SHOOTABLE;
				//}
				//else
				//{
				//continue;
				//}
				break;
			case ET_CONSTRUCTIBLE_MARKER:
				classInfo.mGroup = ENT_GRP_PROP;
				classInfo.mClassId = ET_CLASSEX_CONSTRUCTIBLE;
				classInfo.mCategory.SetFlag( ENT_CAT_OBSTACLE );
				if ( ent->r.linked )
					classInfo.mFlags.SetFlag( ENT_FLAG_COLLIDABLE );
				break;
			case ET_GENERAL:
			{
				if ( !Q_stricmp( ent->classname, "func_invisible_user" ) )
				{
					classInfo.mGroup = ENT_GRP_BUTTON;
				}
				else if ( !Q_stricmp( ent->classname, "func_button" ) )
				{
					classInfo.mGroup = ENT_GRP_BUTTON;
				}
				else if ( !Q_stricmp( ent->classname, "misc_mg42" ) )
				{
					classInfo.mGroup = ENT_GRP_MOUNTABLE;
					classInfo.mClassId = ET_CLASSEX_MG42MOUNT;
				}
				else if ( !Q_stricmp( ent->classname, "misc_mg42base" ) )
				{
					classInfo.mGroup = ENT_GRP_PROP;
					classInfo.mClassId = ET_CLASSEX_MG42BASE;
					classInfo.mCategory.SetFlag( ENT_CAT_OBSTACLE );
					classInfo.mFlags.SetFlag( ENT_FLAG_COLLIDABLE );
				}
				else if ( !Q_stricmp( ent->classname, "props_chair_hiback" ) )
				{
					classInfo.mGroup = ENT_GRP_PROP;
					classInfo.mClassId = ET_CLASSEX_BROKENCHAIR;
					classInfo.mCategory.SetFlag( ENT_CAT_OBSTACLE );
					classInfo.mFlags.SetFlag( ENT_FLAG_COLLIDABLE );
				}
				else if ( !Q_stricmp( ent->classname, "props_chair" ) )
				{
					classInfo.mGroup = ENT_GRP_PROP;
					classInfo.mClassId = ET_CLASSEX_BROKENCHAIR;
					classInfo.mFlags.SetFlag( ENT_FLAG_COLLIDABLE );
				}
				else if ( !Q_stricmp( ent->classname, "props_chair_side" ) )
				{
					classInfo.mGroup = ENT_GRP_PROP;
					classInfo.mClassId = ET_CLASSEX_BROKENCHAIR;
					classInfo.mCategory.SetFlag( ENT_CAT_OBSTACLE );
					classInfo.mFlags.SetFlag( ENT_FLAG_COLLIDABLE );
				}
				// cs: waypoint tool, don't merge the spawns
				else if ( !Q_stricmp( ent->classname, "info_player_deathmatch" ) ||
					!Q_stricmp( ent->classname, "team_CTF_redspawn" ) ||
					!Q_stricmp( ent->classname, "team_CTF_bluespawn" ) ||
					!Q_stricmp( ent->classname, "info_player_spawn" ) )
				{
					classInfo.mGroup = ENT_GRP_PLAYERSTART;
				}
				else if ( !Q_stricmp( ent->classname, "func_constructible" ) )
				{
					classInfo.mGroup = ENT_GRP_PROP;
					classInfo.mClassId = ET_CLASSEX_CONSTRUCTIBLE;
					classInfo.mCategory.SetFlag( ENT_CAT_OBSTACLE );
					classInfo.mFlags.SetFlag( ENT_FLAG_COLLIDABLE );
				}
				break;
			}
			case ET_MG42_BARREL:
			{
				classInfo.mGroup = ENT_GRP_MOUNTABLE;
				classInfo.mClassId = ET_CLASSEX_MG42MOUNT;

				if ( !Q_stricmp( ent->classname, "misc_mg42" ) )
				{
					if ( Simple_EmplacedGunIsRepairable( ent ) )
					{
						classInfo.mHealth.Set( 0, MG42_MULTIPLAYER_HEALTH );
					}
					else
					{
						if ( ent->mg42BaseEnt > 0 )
						{
							gentity_t *pBase = &g_entities[ ent->mg42BaseEnt ];
							classInfo.mHealth.Set( pBase->health, MG42_MULTIPLAYER_HEALTH );
						}
						else
						{
							classInfo.mHealth.Set( ent->health, MG42_MULTIPLAYER_HEALTH );
						}
					}
				}
				break;
			}
			case ET_TRAP:
			{
				if ( !Q_stricmp( ent->classname, "team_WOLF_checkpoint" ) )
				{
					classInfo.mGroup = ENT_GRP_PROP;
				}
				break;
			}
			case ET_GAMEMODEL:
			{
				classInfo.mGroup = ENT_GRP_PROP;
				classInfo.mCategory.SetFlag( ENT_CAT_OBSTACLE );
				classInfo.mFlags.SetFlag( ENT_FLAG_COLLIDABLE );
				break;
			}
			case ET_INVISIBLE:
			{
				if ( ent->client )
					return ENT_GRP_SPECTATOR;
				break;
			}
			case ET_ITEM:
			{
				if ( !Q_strncmp( ent->classname, "item_health", strlen( "item_health" ) ) )
				{
					classInfo.mGroup = ENT_GRP_RESUPPLY;
					classInfo.mHealth.Set( 20 ); // fixme?
				}
				else if ( !Q_strncmp( ent->classname, "weapon_magicammo", strlen( "weapon_magicammo" ) ) )
				{
					classInfo.mGroup = ENT_GRP_RESUPPLY;
					classInfo.mAmmo[ 0 ].Set( 20, 0 );
				}
				else if ( !Q_stricmp( ent->classname, "item_treasure" ) )
				{
					classInfo.mGroup = ENT_GRP_GOAL;
				}
				else if ( ent->item && ent->item->giType == IT_WEAPON )
				{
					classInfo.mGroup = ENT_GRP_WEAPON;
					classInfo.mClassId = Bot_WeaponGameToBot( ent->item->giTag );
				}
				break;
			}
			case ET_CORPSE:
			{
				classInfo.mGroup = ENT_GRP_PROP_STATIC;
				classInfo.mClassId = ET_CLASSEX_CORPSE;
				break;
			}
			case ET_MISSILE:
			{
				classInfo.mGroup = ENT_GRP_PROJECTILE;

				// Register certain weapons as threats to avoid or whatever.
				switch ( ent->s.weapon )
				{
					case WP_GRENADE_LAUNCHER:
					case WP_GRENADE_PINEAPPLE:
						classInfo.mClassId = ET_CLASSEX_GRENADE;
						break;
					case WP_PANZERFAUST:
						classInfo.mClassId = ET_CLASSEX_ROCKET;
						break;
#ifdef NOQUARTER
					case WP_BAZOOKA:
						classInfo.mClassId = ET_CLASSEX_ROCKET;
						break;
#endif
					case WP_ARTY:
						classInfo.mClassId = ET_CLASSEX_ARTY;
						break;
					case WP_DYNAMITE:
						classInfo.mClassId = ET_CLASSEX_DYNAMITE;
						break;
					case WP_SMOKE_MARKER:
						classInfo.mClassId = ET_CLASSEX_SMOKEMARKER;
						break;
					case WP_SMOKE_BOMB:
						classInfo.mClassId = ET_CLASSEX_SMOKEBOMB;
						break;
					case WP_LANDMINE:
						classInfo.mClassId = ET_CLASSEX_MINE;
						break;
					case WP_SATCHEL:
						classInfo.mClassId = ET_CLASSEX_SATCHEL;
						break;
					case WP_M7:
						classInfo.mClassId = ET_CLASSEX_M7_GRENADE;
						break;
					case WP_GPG40:
						classInfo.mClassId = ET_CLASSEX_GPG40_GRENADE;
						break;
					case WP_MORTAR_SET:
						classInfo.mClassId = ET_CLASSEX_MORTAR;
						break;
#ifdef NOQUARTER
					case WP_MORTAR2_SET:
						classInfo.mClassId = ET_CLASSEX_MORTAR;
#endif
					default:
						if ( !Q_strncmp( ent->classname, "air strike", sizeof( "air strike" ) ) )
						{
							classInfo.mClassId = ET_CLASSEX_AIRSTRIKE;
							break;
						}
				}
				break;
			case ET_FLAMETHROWER_CHUNK:
			{
				classInfo.mGroup = ENT_GRP_PROJECTILE;
				classInfo.mClassId = ET_CLASSEX_FLAMECHUNK;
				break;
			}
			//case ET_AAGUN:
			//	{
			//		if((pCurrent->health > 0) &&
			//		(pCurrent->entstate != STATE_INVISIBLE) &&
			//		(pCurrent->entstate != STATE_UNDERCONSTRUCTION))
			//		{
			//		}
			//		break;
			//	}
			case ET_EXPLOSIVE:
			{
				if ( !( ent->spawnflags & EXPLOSIVE_TANK ) &&
					( ent->constructibleStats.weaponclass != 1 ) &&
					( ent->constructibleStats.weaponclass != 2 ) )// &&
					//(_ent->health > 0) && (_ent->takedamage == qtrue))
				{
					return ET_CLASSEX_BREAKABLE;
				}
				break;
			}
			case ET_HEALER:
			{
				classInfo.mGroup = ENT_GRP_DISPENSER;
				classInfo.mClassId = ET_CLASSEX_HEALTHCABINET;
				classInfo.mCategory.SetFlag( ENT_CAT_OBSTACLE );
				classInfo.mFlags.SetFlag( ENT_FLAG_COLLIDABLE );
				break;
			}
			case ET_SUPPLIER:
			{
				classInfo.mGroup = ENT_GRP_DISPENSER;
				classInfo.mClassId = ET_CLASSEX_AMMOCABINET;
				classInfo.mCategory.SetFlag( ENT_CAT_OBSTACLE );
				classInfo.mFlags.SetFlag( ENT_FLAG_COLLIDABLE );
				break;
			}
			case ET_OID_TRIGGER:
			{
				classInfo.mGroup = ENT_GRP_GOAL;
				/*classInfo.mCategory.SetFlag( ENT_CAT_OBSTACLE );
				classInfo.mFlags.SetFlag( ENT_FLAG_COLLIDABLE );*/
				break;
			}
			}
		}
	}
	return classInfo.mGroup != ENT_GRP_UNKNOWN;
}
