local CameraController = {
    properties = {
        speed = { type = "float", default = 6.0, min = 0.0, max = 64.0 },
        verticalSpeed = { type = "float", default = 4.0, min = 0.0, max = 32.0 },
        boostMultiplier = { type = "float", default = 2.5, min = 1.0, max = 10.0 }
    }
}

local Input = Input or {}
local Key = Input.Key or {}

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

function CameraController:awake()
    self.movement = { x = 0.0, y = 0.0, z = 0.0 }
end

function CameraController:update(deltaTime)
    local props = self.properties or {}
    self.movement.x = axis(Key.D, Key.A)
    self.movement.y = axis(Key.E, Key.Q)
    self.movement.z = axis(Key.W, Key.S)

    if self.movement.x == 0.0 and self.movement.y == 0.0 and self.movement.z == 0.0 then
        return
    end

    self.movement = normalize(self.movement)
    local currentSpeed = props.speed or 6.0
    if Input.IsKeyDown(Key.LEFT_SHIFT) or Input.IsKeyDown(Key.RIGHT_SHIFT) then
        currentSpeed = currentSpeed * (props.boostMultiplier or 2.5)
    end

    Entity.Translate(
        self.movement.x * currentSpeed * deltaTime,
        self.movement.y * (props.verticalSpeed or 4.0) * deltaTime,
        self.movement.z * currentSpeed * deltaTime)
end

return CameraController
