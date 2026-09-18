package com.polyrhythm.metronome.data

import androidx.room.*

@Entity(tableName = "metronome_presets")
data class MetronomePresetEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val name: String,
    val bpm: Double,
    val timeSignatureNumerator: Int,
    val timeSignatureDenominator: Int,
    val layersJson: String, // Katman konfigürasyonlarının JSON serileştirmesi
    val tempoTrainerEnabled: Boolean = false,
    val muteTrainerEnabled: Boolean = false,
    val createdAt: Long = System.currentTimeMillis()
)

@Dao
interface MetronomePresetDao {
    @Query("SELECT * FROM metronome_presets ORDER BY createdAt DESC")
    suspend fun getAllPresets(): List<MetronomePresetEntity>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertPreset(preset: MetronomePresetEntity): Long

    @Delete
    suspend fun deletePreset(preset: MetronomePresetEntity)
}

@Database(entities = [MetronomePresetEntity::class], version = 1, exportSchema = false)
abstract class MetronomeDatabase : RoomDatabase() {
    abstract fun presetDao(): MetronomePresetDao
}
