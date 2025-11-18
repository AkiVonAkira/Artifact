local Input = Input or {}
local Key = Input.Key or {}

local speed = 6.0
local verticalSpeed = 4.0
local boostMultiplier = 2.5

local function normalize(vec)
    local mag = math.sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z)
    if mag <= 0.0001 then
        return vec
    end
    return { x = vec.x / mag, y = vec.y / mag, z = vec.z / mag }
end

local function axis(positiveKey, negativeKey)
    local value = 0.0
    if Input.IsKeyDown(positiveKey or 0) then
        value = value + 1.0
    end
    if Input.IsKeyDown(negativeKey or 0) then
        value = value - 1.0
    end
    return value
end

local movement = {
    x = axis(Key.D, Key.A),
    y = axis(Key.E, Key.Q),
    z = axis(Key.W, Key.S)
}

if movement.x ~= 0.0 or movement.y ~= 0.0 or movement.z ~= 0.0 then
    movement = normalize(movement)
    local dt = Time.deltaTime or 0.0
    local currentSpeed = speed
    if Input.IsKeyDown(Key.LEFT_SHIFT) or Input.IsKeyDown(Key.RIGHT_SHIFT) then
        currentSpeed = currentSpeed * boostMultiplier
    end
    Entity.Translate(movement.x * currentSpeed * dt, movement.y * verticalSpeed * dt, movement.z * currentSpeed * dt)
end
