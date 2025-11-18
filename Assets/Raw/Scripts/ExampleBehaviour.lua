local ExampleBehaviour = {
    properties = {
        greeting = { type = "string", default = "Hello Artifact" },
        isVerbose = { type = "bool", default = true },
        spinSpeed = { type = "float", default = 30.0, min = 0.0, max = 360.0 }
    }
}

function ExampleBehaviour:awake()
    if self.properties.isVerbose then
        print("[Lua] ExampleBehaviour.awake")
    end
end

function ExampleBehaviour:start()
    if self.properties.isVerbose then
        print(string.format("[Lua] start -> greeting='%s'", self.properties.greeting))
    end
end

function ExampleBehaviour:update(deltaTime)
    -- Rotate a touch every frame so we can see values taking effect.
    local rotation = Entity.GetPosition()
    rotation.y = rotation.y + (self.properties.spinSpeed or 0.0) * deltaTime
    Entity.SetPosition(rotation.x, rotation.y, rotation.z)
end

return ExampleBehaviour
