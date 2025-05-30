-- Set timezone to Madrid (Europe/Madrid)
SET timezone = 'Europe/Madrid';

-- Función para actualizar el campo updated_at automáticamente
CREATE OR REPLACE FUNCTION UPDATE_UPDATED_AT_COLUMN()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;
