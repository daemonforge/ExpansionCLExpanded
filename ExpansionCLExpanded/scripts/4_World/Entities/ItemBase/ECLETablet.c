class ECLETablet extends ItemBase{
	
	protected bool m_HackingStarted = false;
	protected bool m_HackingInterrupted = false;
	protected bool m_HackingInterruptedLocal = false;
	
	protected bool m_HackingCompleted = false;
	protected bool m_HackingCompletedLocal = false;
	
	protected bool m_TabletON = false;
	protected bool m_TabletONLocal = false;
	
	protected float m_HackTimeRemaining;
		
	override void SetActions()
	{
		super.SetActions();

        AddAction(ActionHackExpansionCodeLockOnTent);
        AddAction(ActionHackExpansionCodeLockOnDoor);
    }

	void ECLETablet(){
		
	}
	
	void ~ECLETablet()
	{
		
	}

	bool HasHackingStarted(){
		return m_HackingStarted;
	}

	bool WasHackingInterrupted(){
		return m_HackingInterrupted;
	}
		
	override void InitItemVariables()
	{
		super.InitItemVariables();
		RegisterNetSyncVariableBool("m_HackingStarted");
		RegisterNetSyncVariableBool("m_HackingInterrupted");
		RegisterNetSyncVariableFloat("m_HackTimeRemaining");
		RegisterNetSyncVariableBool("m_HackingCompleted");
	}
	
	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		
		if ( m_HackingInterrupted && !m_HackingInterruptedLocal )
		{
			HackInterruptedClient();
		} else if ( !m_HackingInterrupted){
			m_HackingInterruptedLocal = m_HackingInterrupted;
		}
		
		if ( m_HackingCompleted && !m_HackingCompletedLocal )
		{
			HackCompletedClient();
		} else if ( !m_HackingCompleted){
			m_HackingCompletedLocal = m_HackingCompleted;
		}
		
		if ( m_TabletON && !m_TabletONLocal ) {
			TurnOnTablet();
		} else if ( !m_TabletON && m_TabletONLocal ) {
			TurnOffTablet();
		}
		
	}
	
	void HackInterruptedClient(){
		m_HackingInterruptedLocal = m_HackingInterrupted;
		SEffectManager.PlaySoundOnObject("landmine_end_SoundSet", this);
	}
	
	void HackCompletedClient(){
		m_HackingCompletedLocal = m_HackingCompleted;
		SEffectManager.PlaySoundOnObject("Expansion_CodeLock_Unlock_SoundSet", this);
	}
	
	
	void StartHackServer(ItemBase hackingTarget, PlayerBase hacker){
		PlayerBase Hacker = PlayerBase.Cast(hacker);
		ItemBase HackingTarget = ItemBase.Cast(hackingTarget);
		if (Hacker && HackingTarget){
			m_HackingCompleted = false;
			m_HackingInterrupted = false;
			if (!m_HackingStarted){
				
				
				m_HackingStarted = true;
				m_HackTimeRemaining = GetExpansionCodeLockConfig().HackingTimeTents * 1000;
				if( BaseBuildingBase.Cast(hackingTarget)){
					m_HackTimeRemaining = GetExpansionCodeLockConfig().HackingTimeDoors * 1000;
				}
			}
			GetGame().GetCallQueue( CALL_CATEGORY_SYSTEM ).CallLater(this.CheckHackProgress, 2000, false, hackingTarget, hacker);
			SetSynchDirty();
		}
	}
	
	void StartHackClient(){
		SEffectManager.PlaySoundOnObject("defibrillator_ready_SoundSet", this);
	}
	
	void CheckHackProgress(ItemBase hackingTarget, PlayerBase hacker){
		PlayerBase Hacker = PlayerBase.Cast(hacker);
		ItemBase HackingTarget = ItemBase.Cast(hackingTarget);
		if (Hacker && HackingTarget){
			float DoInterrupt = Math.RandomFloat(0,1);
			float InterruptChance = GetExpansionCodeLockConfig().ChanceOfInterrupt; // so the chance is effectly every 4 seconds instead of every 2 Seconds
			TentBase tent = TentBase.Cast(hackingTarget); 
			BaseBuildingBase door = BaseBuildingBase.Cast(hackingTarget);
			
			if(door && CountBatteries() < GetExpansionCodeLockConfig().BatteriesDoors){
				m_HackingInterrupted = true;
			}
			if(tent && CountBatteries() < GetExpansionCodeLockConfig().BatteriesTents){
				m_HackingInterrupted = true;
			}
			
			if (DoInterrupt < InterruptChance){
				m_HackingInterrupted = true;
			}
			
			if (!m_HackingInterrupted && !HackingTarget.IsRuined() && vector.Distance(HackingTarget.GetPosition(), Hacker.GetPosition()) < 10 && Hacker.GetItemInHands() == this){
				m_HackTimeRemaining = m_HackTimeRemaining - 2000;
				if(m_HackTimeRemaining > 2000){
					GetGame().GetCallQueue( CALL_CATEGORY_SYSTEM ).CallLater(this.CheckHackProgress, 2000, false, hackingTarget, Hacker);
				}else if (m_HackTimeRemaining > 500){ //if its this close to finishing just finish
					GetGame().GetCallQueue( CALL_CATEGORY_SYSTEM ).CallLater(this.CheckHackProgress, m_HackTimeRemaining, false, hackingTarget, Hacker);
				} else {
					m_HackTimeRemaining = 0; //incase it was made negitive
					GetGame().GetCallQueue( CALL_CATEGORY_SYSTEM ).CallLater(this.HackCompleted, 200, false, hackingTarget, Hacker);
				}
			} else {
				m_HackingInterrupted = true;
				if(Hacker.GetIdentity()){
					string InterruptedHeading = "Hack Interrupted";
					string InterruptedMessage = "The Hacking of " + hackingTarget.GetDisplayName() + " has been Interrupted";
					string InterruptedIcon = "ExpansionCLExpanded/GUI/Images/hacking.paa";
					GetNotificationSystem().CreateNotification(new ref StringLocaliser(InterruptedHeading), new ref StringLocaliser(InterruptedMessage), InterruptedIcon, -938286307, 15, Hacker.GetIdentity());
				}
			}
		}else{
			m_HackingStarted = false;
			m_HackingInterrupted = false;
		}
		SetSynchDirty();
	}
	
	void HackCompleted(ItemBase hackingTarget, PlayerBase hacker){
		PlayerBase Hacker = PlayerBase.Cast(hacker);
		BaseBuildingBase HackingTarget = BaseBuildingBase.Cast(hackingTarget);
		float itemMaxHealth = 0;
		float toolDamage = GetExpansionCodeLockConfig().HackSawDamage;
        TentBase tent = TentBase.Cast(hackingTarget); 
        if (tent && Hacker) {
            ExpansionCodeLock codelock = ExpansionCodeLock.Cast(tent.FindAttachmentBySlotName( "Att_ExpansionCodeLock" ));
			
            if (codelock ) {
				itemMaxHealth = codelock.GetMaxHealth("", "Health");
				itemMaxHealth++;
				codelock.AddHealth("", "Health", -itemMaxHealth);
				toolDamage++;
                tent.Unlock();
                tent.SetCode( "" );
				tent.GetInventory().DropEntity(InventoryMode.SERVER, tent, codelock);
				#ifdef HEROESANDBANDITSMOD
					if (Hacker.GetIdentity()){
						GetHeroesAndBandits().NewPlayerAction(Hacker.GetIdentity().GetPlainId(), "HackExpansionCodeLockDoorRaid");
					}
				#endif
				if (Hacker.GetIdentity() && GetExpansionCodeLockConfig().ScriptLogging){
					Print("[CodeLockExpanded][Raid] " + Hacker.GetIdentity().GetName() + "(" +  Hacker.GetIdentity().GetPlainId() + ") Hacked  " + tent.GetType() + " at " + tent.GetPosition());
				}
				this.AddHealth("GlobalHealth", "Health", GetExpansionCodeLockConfig().TabletDamage);
				DestoryBatteries( GetExpansionCodeLockConfig().BatteriesTents );
				m_HackingStarted = false;
				m_HackingInterrupted = false;
				m_HackingCompleted = true;
            }
        }
		if (HackingTarget && Hacker) {
            ExpansionCodeLock codelock2 = ExpansionCodeLock.Cast(HackingTarget.GetAttachmentByConfigTypeName("ExpansionCodeLock"));
			
            if (codelock2 ) {
				itemMaxHealth = codelock2.GetMaxHealth("", "Health");
				itemMaxHealth++;
				codelock2.AddHealth("", "Health", -itemMaxHealth);
				toolDamage++;
                HackingTarget.Unlock();
                HackingTarget.SetCode( "" );
				HackingTarget.GetInventory().DropEntity(InventoryMode.SERVER, HackingTarget, codelock2);
				#ifdef HEROESANDBANDITSMOD
					if (Hacker.GetIdentity()){
						GetHeroesAndBandits().NewPlayerAction(Hacker.GetIdentity().GetPlainId(), "HackExpansionCodeLockTentRaid");
					}
				#endif
				if (Hacker.GetIdentity() && GetExpansionCodeLockConfig().ScriptLogging){
					Print("[CodeLockExpanded][Raid] " + Hacker.GetIdentity().GetName() + "(" +  Hacker.GetIdentity().GetPlainId() + ") Hacked  " + HackingTarget.GetType() + " at " + HackingTarget.GetPosition());
				}
				this.AddHealth("", "Health", GetExpansionCodeLockConfig().TabletDamage);
				DestoryBatteries( GetExpansionCodeLockConfig().BatteriesDoors );
				m_HackingStarted = false;
				m_HackingInterrupted = false;
				m_HackingCompleted = true;
            }
        }
		if (CountBatteries() < 1){
			m_TabletON = false;
		}
		if (m_HackingCompleted && Hacker.GetIdentity()){
			string CompletedHeading = "Hack Finished";
			string CompletedMessage = "The Hacking of " + hackingTarget.GetDisplayName() + " has Finished";
			string CompletedIcon = "ExpansionCLExpanded/GUI/Images/hacking.paa";
			GetNotificationSystem().CreateNotification(new ref StringLocaliser(CompletedHeading), new ref StringLocaliser(CompletedMessage), CompletedIcon, -938286307, 15, Hacker.GetIdentity());
		}
		SetSynchDirty();
	}
	
	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);	
		AddHealth( "GlobalHealth", "Health", -damageResult.GetDamage( "", "Health" ) );
	}

	void TurnOnTablet(){
		m_TabletON = true;
		m_TabletONLocal = true;
		SetObjectMaterial( 0, "ExpansionCLExpanded\\Data\\textures\\ECLE_Tablet_On.rvmat" );
		SetObjectMaterial( 1, "ExpansionCLExpanded\\Data\\textures\\ECLE_Tablet_On.rvmat" );
		SetObjectTexture( 0, "ExpansionCLExpanded\\Data\\textures\\ECLE_Tablet_On.paa" );
		SetObjectTexture( 1, "ExpansionCLExpanded\\Data\\textures\\ECLE_Tablet_On.paa" );
	}
	
	void TurnOffTablet(){
		m_TabletON = false;
		m_TabletONLocal = false;
		SetObjectMaterial( 0, "ExpansionCLExpanded\\Data\\textures\\ECLE_Tablet_Good.rvmat" );
		SetObjectMaterial( 1, "ExpansionCLExpanded\\Data\\textures\\ECLE_Tablet_Good.rvmat" );
		SetObjectTexture( 0, "ExpansionCLExpanded\\Data\\textures\\ECLE_Tablet_Good.paa" );
		SetObjectTexture( 1, "ExpansionCLExpanded\\Data\\textures\\ECLE_Tablet_Good.paa" );
	}
	
	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave( ctx );
		
		ctx.Write( m_HackingStarted );
		ctx.Write( m_HackingInterrupted );
		ctx.Write( m_HackingCompleted );
		ctx.Write( m_HackTimeRemaining );
		ctx.Write( m_TabletON );
	}

	

	override bool OnStoreLoad( ParamsReadContext ctx, int version )
	{
		if ( !super.OnStoreLoad( ctx, version ) )
		{
			return false;
		}
		bool loadingsuccessfull = true;
		if ( !ctx.Read( m_HackingStarted ) )
		{
			loadingsuccessfull = false;
		}
		if ( !ctx.Read( m_HackingInterrupted ) )
		{
			loadingsuccessfull = false;
		}
		if ( !ctx.Read( m_HackingCompleted ) )
		{
			loadingsuccessfull = false;
		}
		if ( !ctx.Read( m_HackTimeRemaining ) )
		{
			loadingsuccessfull = false;
		}
		if ( !ctx.Read( m_TabletON ) )
		{
			loadingsuccessfull = false;
		}
		
		
		
		if ( loadingsuccessfull && m_HackingStarted && m_HackTimeRemaining > 0 && !m_HackingCompleted ){
			m_HackingInterrupted = true;
		}
				
		SetSynchDirty();
		
		return loadingsuccessfull;
	}
	
	int CountBatteries(){
		int count = 0;
		ECLETabletBattery tempBattery1;
		ECLETabletBattery tempBattery2;
		ECLETabletBattery tempBattery3;
		
		if (Class.CastTo(tempBattery1, FindAttachmentBySlotName("Att_ECLETabletBattery_1"))){
			if (!tempBattery1.IsRuined()){
				count++;
			}
		}
		if (Class.CastTo(tempBattery2, FindAttachmentBySlotName("Att_ECLETabletBattery_2"))){
			if (!tempBattery2.IsRuined()){
				count++;
			}
		}
		if (Class.CastTo(tempBattery3, FindAttachmentBySlotName("Att_ECLETabletBattery_3"))){
			if (!tempBattery3.IsRuined()){
				count++;
			}
		}
		return count;
	}
	
	void DestoryBatteries( int number){
		int count = 0;
		ECLETabletBattery tempBattery1;
		ECLETabletBattery tempBattery2;
		ECLETabletBattery tempBattery3;
		
		if (Class.CastTo(tempBattery3, FindAttachmentBySlotName("Att_ECLETabletBattery_3"))){
			if (!tempBattery3.IsRuined()){
				tempBattery3.AddHealth("", "Health", -21);
				count++;
			}
			if (count >= number){
				return;
			}
		}
		if (Class.CastTo(tempBattery2, FindAttachmentBySlotName("Att_ECLETabletBattery_2"))){
			if (!tempBattery2.IsRuined()){
				tempBattery2.AddHealth("", "Health", -21);
				count++;
			}
			if (count >= number){
				return;
			}
		}
		if (Class.CastTo(tempBattery1, FindAttachmentBySlotName("Att_ECLETabletBattery_1"))){
			if (!tempBattery1.IsRuined()){
				tempBattery1.AddHealth("", "Health", -21);
				count++;
			}
			if (count >= number){
				return;
			}
		}
	}
	
	override void EEItemAttached(EntityAI item, string slot_name){
		super.EEItemAttached(item, slot_name);
		if (slot_name == "Att_ECLETabletBattery_1" || slot_name == "Att_ECLETabletBattery_2" || slot_name == "Att_ECLETabletBattery_3" ){
			m_TabletON = true;
		}
	}
	
	override void EEItemDetached(EntityAI item, string slot_name)
	{
		super.EEItemDetached(item, slot_name);
		if (slot_name == "Att_ECLETabletBattery_1" || slot_name == "Att_ECLETabletBattery_2" || slot_name == "Att_ECLETabletBattery_3" ){
			if (CountBatteries() < 1){
				m_TabletON = false;
			}
		}
		
	}
	
}